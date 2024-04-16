// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <boost/log/trivial.hpp>
#include <glm/ext.hpp>
#include <vma/vk_mem_alloc.h>
#include <Volk/volk.h>

#ifndef TINYGLTF_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#endif
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#include <execution>
#include <tiny_gltf.h>

module RenderCore.Runtime.Scene;

import RenderCore.Types.Object;
import RenderCore.Types.Mesh;
import RenderCore.Types.Material;
import RenderCore.Types.Texture;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Types.SurfaceProperties;
import RenderCore.Runtime.Device;
import RenderCore.Runtime.Command;
import RenderCore.Runtime.Memory;
import RenderCore.Runtime.Model;
import RenderCore.Runtime.SwapChain;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;

using namespace RenderCore;

Camera                                              g_Camera {};
Illumination                                        g_Illumination {};
std::pair<BufferAllocation, VkDescriptorBufferInfo> m_UniformBufferAllocation {};

VkSampler                            g_Sampler { VK_NULL_HANDLE };
ImageAllocation                      g_DepthImage {};
ImageAllocation                      g_EmptyImage {};
std::atomic<std::uint64_t>           g_ObjectAllocationIDCounter { 0U };
std::vector<std::shared_ptr<Object>> g_Objects {};
std::mutex                           g_ObjectMutex {};

void RenderCore::CreateSceneUniformBuffer()
{
    constexpr VkDeviceSize BufferSize = sizeof(SceneUniformData);
    CreateUniformBuffers(m_UniformBufferAllocation.first, BufferSize, "SCENE_UNIFORM");
    m_UniformBufferAllocation.second = { .buffer = m_UniformBufferAllocation.first.Buffer, .offset = 0U, .range = BufferSize };
}

void RenderCore::CreateImageSampler()
{
    CreateTextureSampler(GetPhysicalDevice(), g_Sampler);
}

void RenderCore::CreateDepthResources(SurfaceProperties const &SurfaceProperties)
{
    if (g_DepthImage.IsValid())
    {
        g_DepthImage.DestroyResources(GetAllocator());
    }

    constexpr VkImageUsageFlagBits Usage  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    constexpr VkImageAspectFlags   Aspect = VK_IMAGE_ASPECT_DEPTH_BIT;

    g_DepthImage.Format = SurfaceProperties.DepthFormat;

    CreateImage(g_DepthImage.Format,
                SurfaceProperties.Extent,
                g_ImageTiling,
                Usage,
                g_TextureMemoryUsage,
                "DEPTH",
                g_DepthImage.Image,
                g_DepthImage.Allocation);

    CreateImageView(g_DepthImage.Image,
                    g_DepthImage.Format,
                    DepthHasStencil(g_DepthImage.Format) ? Aspect | VK_IMAGE_ASPECT_STENCIL_BIT : Aspect,
                    g_DepthImage.View);
}

void RenderCore::AllocateEmptyTexture(VkFormat const TextureFormat)
{
    constexpr std::uint32_t                                DefaultTextureHalfSize { 2U };
    constexpr std::uint32_t                                DefaultTextureSize { DefaultTextureHalfSize * DefaultTextureHalfSize };
    constexpr std::array<std::uint8_t, DefaultTextureSize> DefaultTextureData {};

    VkCommandPool                CopyCommandPool { VK_NULL_HANDLE };
    std::vector<VkCommandBuffer> CommandBuffers(1U, VK_NULL_HANDLE);

    auto const &[FamilyIndex, Queue] = GetGraphicsQueue();

    InitializeSingleCommandQueue(CopyCommandPool, CommandBuffers, FamilyIndex);
    const auto [Index, Buffer, Allocation] = AllocateTexture(CommandBuffers.at(0U),
                                                             std::data(DefaultTextureData),
                                                             DefaultTextureHalfSize,
                                                             DefaultTextureHalfSize,
                                                             TextureFormat,
                                                             DefaultTextureSize * DefaultTextureSize);

    FinishSingleCommandQueue(Queue, CopyCommandPool, CommandBuffers);

    vmaDestroyBuffer(GetAllocator(), Buffer, Allocation);
}

void RenderCore::LoadScene(std::string_view const ModelPath)
{
    tinygltf::Model Model {};
    {
        tinygltf::TinyGLTF          ModelLoader {};
        std::string                 Error {};
        std::string                 Warning {};
        std::filesystem::path const ModelFilepath(ModelPath);
        bool const                  LoadResult = ModelFilepath.extension() == ".gltf"
                                                     ? ModelLoader.LoadASCIIFromFile(&Model, &Error, &Warning, std::data(ModelPath))
                                                     : ModelLoader.LoadBinaryFromFile(&Model, &Error, &Warning, std::data(ModelPath));
        if (!std::empty(Error))
        {
            BOOST_LOG_TRIVIAL(error) << "[" << __func__ << "]: Error: '" << Error << "'";
        }

        if (!std::empty(Warning))
        {
            BOOST_LOG_TRIVIAL(warning) << "[" << __func__ << "]: Warning: '" << Warning << "'";
        }

        if (!LoadResult)
        {
            BOOST_LOG_TRIVIAL(error) << "[" << __func__ << "]: Failed to load model from path: '" << ModelPath << "'";
            return;
        }
    }


    VkCommandPool                CopyCommandPool { VK_NULL_HANDLE };
    std::vector<VkCommandBuffer> CommandBuffers { VK_NULL_HANDLE };

    auto const &                                                [QueueIndex, Queue] = GetGraphicsQueue();
    std::unordered_map<VkBuffer, VmaAllocation>                 BufferAllocations {};
    std::unordered_map<std::uint32_t, std::shared_ptr<Texture>> TextureMap {};

    InitializeSingleCommandQueue(CopyCommandPool, CommandBuffers, QueueIndex);
    {
        VkCommandBuffer &CommandBuffer = CommandBuffers.at(0U);

        for (std::uint32_t Iterator = 0U; Iterator < std::size(Model.textures); ++Iterator)
        {
            tinygltf::Texture const &TextureIter = Model.textures.at(Iterator);
            if (TextureIter.source < 0)
            {
                continue;
            }

            if (TextureMap.contains(Iterator))
            {
                continue;
            }

            tinygltf::Image const &ImageIter = Model.images.at(TextureIter.source);

            if (std::empty(ImageIter.image))
            {
                continue;
            }

            std::uint32_t const TextureID   = g_ObjectAllocationIDCounter.fetch_add(1U);
            std::string const   TextureName = std::format("{}_{:03d}", std::empty(ImageIter.name) ? "None" : ImageIter.name, TextureID);
            auto                NewTexture  = std::shared_ptr<Texture>(new Texture { TextureID, ImageIter.uri, TextureName }, TextureDeleter {});

            auto const [Index, Buffer, Allocation] = AllocateTexture(CommandBuffer,
                                                                     std::data(ImageIter.image),
                                                                     ImageIter.width,
                                                                     ImageIter.height,
                                                                     ImageIter.component == 3 ? VK_FORMAT_R8G8B8_UNORM : VK_FORMAT_R8G8B8A8_UNORM,
                                                                     ImageIter.image.size());

            BufferAllocations.emplace(Buffer, Allocation);

            NewTexture->SetBufferIndex(Index);
            TextureMap.emplace(Iterator, std::move(NewTexture));
        }

        for (tinygltf::Node const &Node : Model.nodes)
        {
            std::int32_t const MeshIndex = Node.mesh;
            if (MeshIndex < 0)
            {
                continue;
            }

            for (tinygltf::Mesh const &     LoadedMesh = Model.meshes.at(MeshIndex);
                 tinygltf::Primitive const &PrimitiveIter : LoadedMesh.primitives)
            {
                std::uint32_t const MeshID = g_ObjectAllocationIDCounter.fetch_add(1U);

                std::string const MeshName = std::format("{}_{:03d}", std::empty(LoadedMesh.name) ? "None" : LoadedMesh.name, MeshID);
                auto              NewMesh  = std::shared_ptr<Mesh>(new Mesh { MeshID, ModelPath, MeshName }, MeshDeleter {});

                SetVertexAttributes(NewMesh, Model, PrimitiveIter);
                SetPrimitiveTransform(NewMesh, Node);
                AllocatePrimitiveIndices(NewMesh, Model, PrimitiveIter);

                if (PrimitiveIter.material < 0)
                {
                    continue;
                }

                tinygltf::Material const &MeshMaterial = Model.materials.at(PrimitiveIter.material);

                NewMesh->SetMaterialData({
                                                 .BaseColorFactor = glm::make_vec4(std::data(MeshMaterial.pbrMetallicRoughness.baseColorFactor)),
                                                 .EmissiveFactor = glm::make_vec3(std::data(MeshMaterial.emissiveFactor)),
                                                 .MetallicFactor = static_cast<float>(MeshMaterial.pbrMetallicRoughness.metallicFactor),
                                                 .RoughnessFactor = static_cast<float>(MeshMaterial.pbrMetallicRoughness.roughnessFactor),
                                                 .AlphaCutoff = static_cast<float>(MeshMaterial.alphaCutoff),
                                                 .NormalScale = static_cast<float>(MeshMaterial.normalTexture.scale),
                                                 .OcclusionStrength = static_cast<float>(MeshMaterial.occlusionTexture.strength),
                                                 .AlphaMode = MeshMaterial.alphaMode == "OPAQUE"
                                                                  ? AlphaMode::ALPHA_OPAQUE
                                                                  : MeshMaterial.alphaMode == "MASK"
                                                                        ? AlphaMode::ALPHA_MASK
                                                                        : AlphaMode::ALPHA_BLEND,
                                                 .DoubleSided = MeshMaterial.doubleSided
                                         });

                auto                                  EmptyTexture = std::make_shared<Texture>(UINT32_MAX, "EMPTY_TEXT", "EMPTY_TEXT");
                std::vector<std::shared_ptr<Texture>> Textures {};

                if (MeshMaterial.pbrMetallicRoughness.baseColorTexture.index >= 0)
                {
                    auto Texture = TextureMap.at(MeshMaterial.pbrMetallicRoughness.baseColorTexture.index);
                    Texture->AppendType(TextureType::BaseColor);
                    Textures.push_back(Texture);
                }
                else
                {
                    EmptyTexture->AppendType(TextureType::BaseColor);
                }

                if (MeshMaterial.normalTexture.index >= 0)
                {
                    auto Texture = TextureMap.at(MeshMaterial.normalTexture.index);
                    Texture->AppendType(TextureType::Normal);
                    Textures.push_back(Texture);
                }
                else
                {
                    EmptyTexture->AppendType(TextureType::Normal);
                }

                if (MeshMaterial.occlusionTexture.index >= 0)
                {
                    auto Texture = TextureMap.at(MeshMaterial.occlusionTexture.index);
                    Texture->AppendType(TextureType::Occlusion);
                    Textures.push_back(Texture);
                }
                else
                {
                    EmptyTexture->AppendType(TextureType::Occlusion);
                }

                if (MeshMaterial.emissiveTexture.index >= 0)
                {
                    auto Texture = TextureMap.at(MeshMaterial.emissiveTexture.index);
                    Texture->AppendType(TextureType::Emissive);
                    Textures.push_back(Texture);
                }
                else
                {
                    EmptyTexture->AppendType(TextureType::Emissive);
                }

                if (MeshMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
                {
                    auto Texture = TextureMap.at(MeshMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index);
                    Texture->AppendType(TextureType::MetallicRoughness);
                    Textures.push_back(Texture);
                }
                else
                {
                    EmptyTexture->AppendType(TextureType::MetallicRoughness);
                }

                if (!std::empty(EmptyTexture->GetTypes()))
                {
                    Textures.push_back(std::move(EmptyTexture));
                }

                NewMesh->SetTextures(std::move(Textures));

                std::uint32_t const ObjectID  = g_ObjectAllocationIDCounter.fetch_add(1U);
                auto                NewObject = std::shared_ptr<Object>(new Object { ObjectID, ModelPath }, ObjectDeleter {});
                NewObject->SetMesh(std::move(NewMesh));
                g_Objects.push_back(std::move(NewObject));
            }
        }

        AllocateModelsBuffers(g_Objects);
    }
    FinishSingleCommandQueue(Queue, CopyCommandPool, CommandBuffers);

    for (auto &[Buffer, Allocation] : BufferAllocations)
    {
        vmaDestroyBuffer(GetAllocator(), Buffer, Allocation);
    }
}

void RenderCore::UnloadObjects(std::vector<std::uint32_t> const &ObjectIDs)
{
    std::lock_guard Lock { g_ObjectMutex };

    std::for_each(std::execution::par_unseq,
                  std::begin(ObjectIDs),
                  std::end(ObjectIDs),
                  [&](std::uint32_t const ObjectIDIter)
                  {
                      if (auto const MatchingIter = std::ranges::find_if(g_Objects,
                                                                         [ObjectIDIter](std::shared_ptr<Object> const &ObjectIter)
                                                                         {
                                                                             return ObjectIter->GetID() == ObjectIDIter;
                                                                         });
                          MatchingIter != std::end(g_Objects))
                      {
                          MatchingIter->get()->Destroy();
                          g_Objects.erase(MatchingIter);
                      }
                  });

    if (std::empty(g_Objects))
    {
        g_ObjectAllocationIDCounter.fetch_sub(g_ObjectAllocationIDCounter.load());
    }
}

void RenderCore::ReleaseSceneResources()
{
    if (g_Sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(volkGetLoadedDevice(), g_Sampler, nullptr);
        g_Sampler = VK_NULL_HANDLE;
    }

    m_UniformBufferAllocation.first.DestroyResources(GetAllocator());
    g_EmptyImage.DestroyResources(GetAllocator());
    g_DepthImage.DestroyResources(GetAllocator());

    DestroyObjects();
}

void RenderCore::DestroyObjects()
{
    std::lock_guard Lock { g_ObjectMutex };

    for (std::shared_ptr<Object> const &Object : g_Objects)
    {
        Object->Destroy();
    }
    g_Objects.clear();

    g_ObjectAllocationIDCounter.fetch_sub(g_ObjectAllocationIDCounter.load());
}

void RenderCore::TickObjects(float const DeltaTime)
{
    std::lock_guard Lock { g_ObjectMutex };

    std::for_each(std::execution::par_unseq,
                  std::begin(g_Objects),
                  std::end(g_Objects),
                  [&, DeltaTime](std::shared_ptr<Object> const &ObjectIter)
                  {
                      if (!ObjectIter->IsPendingDestroy())
                      {
                          ObjectIter->Tick(DeltaTime);
                      }
                  });
}

ImageAllocation const &RenderCore::GetDepthImage()
{
    return g_DepthImage;
}

VkSampler const &RenderCore::GetSampler()
{
    return g_Sampler;
}

ImageAllocation const &RenderCore::GetEmptyImage()
{
    return g_EmptyImage;
}

std::vector<std::shared_ptr<Object>> &RenderCore::GetObjects()
{
    return g_Objects;
}

std::uint32_t RenderCore::GetNumAllocations()
{
    return static_cast<std::uint32_t>(std::size(g_Objects));
}

void *RenderCore::GetSceneUniformData()
{
    return m_UniformBufferAllocation.first.MappedData;
}

VkDescriptorBufferInfo const &RenderCore::GetSceneUniformDescriptor()
{
    return m_UniformBufferAllocation.second;
}

void RenderCore::UpdateSceneUniformBuffer()
{
    if (g_Camera.IsRenderDirty() || g_Illumination.IsRenderDirty())
    {
        constexpr auto SceneUBOSize = sizeof(SceneUniformData);

        SceneUniformData const UpdatedUBO {
                .ProjectionView = g_Camera.GetProjectionMatrix() * g_Camera.GetViewMatrix(),
                .LightPosition = g_Illumination.GetPosition(),
                .LightColor = g_Illumination.GetColor() * g_Illumination.GetIntensity()
        };
        std::memcpy(m_UniformBufferAllocation.first.MappedData, &UpdatedUBO, SceneUBOSize);
    }
}

Camera &RenderCore::GetCamera()
{
    return g_Camera;
}

Illumination &RenderCore::GetIllumination()
{
    return g_Illumination;
}
