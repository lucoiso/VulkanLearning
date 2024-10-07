// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#ifndef TINYGLTF_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#endif
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#include <tiny_gltf.h>

module RenderCore.Runtime.Scene;

import RenderCore.Runtime.Memory;
import RenderCore.Runtime.Device;
import RenderCore.Runtime.Command;
import RenderCore.Factories.Mesh;
import RenderCore.Factories.Texture;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Types.Texture;
import RenderCore.Types.Mesh;
import RenderCore.Utils.Helpers;

using namespace RenderCore;

std::mutex g_ObjectMutex {};

void RenderCore::CreateSceneUniformBuffer()
{
    constexpr VkDeviceSize BufferSize = sizeof(SceneUniformData);
    CreateUniformBuffers(m_UniformBufferAllocation.first, BufferSize, "SCENE_UNIFORM");
    m_UniformBufferAllocation.second = { .buffer = m_UniformBufferAllocation.first.Buffer, .offset = 0U, .range = BufferSize };
}

void RenderCore::CreateImageSampler()
{
    VkPhysicalDeviceProperties SurfaceProperties;
    vkGetPhysicalDeviceProperties(GetPhysicalDevice(), &SurfaceProperties);

    VkSamplerCreateInfo const SamplerCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias = 0.F,
            .anisotropyEnable = false,
            .maxAnisotropy = SurfaceProperties.limits.maxSamplerAnisotropy,
            .compareEnable = false,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .minLod = 0.F,
            .maxLod = FLT_MAX,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = false
    };

    CheckVulkanResult(vkCreateSampler(GetLogicalDevice(), &SamplerCreateInfo, nullptr, &g_Sampler));
}

void RenderCore::CreateDepthResources(SurfaceProperties const &SurfaceProperties)
{
    if (g_DepthImage.IsValid())
    {
        VmaAllocator const &Allocator = GetAllocator();
        g_DepthImage.DestroyResources(Allocator);
    }

    constexpr VkImageUsageFlagBits Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    g_DepthImage.Format                  = SurfaceProperties.DepthFormat;
    VkImageAspectFlags const DepthAspect = DepthHasStencil(g_DepthImage.Format) ? g_DepthAspect | VK_IMAGE_ASPECT_STENCIL_BIT : g_DepthAspect;

    CreateImage(g_DepthImage.Format,
                SurfaceProperties.Extent,
                g_ImageTiling,
                Usage,
                g_TextureMemoryUsage,
                "DEPTH",
                g_DepthImage.Image,
                g_DepthImage.Allocation);

    CreateImageView(g_DepthImage.Image, g_DepthImage.Format, DepthAspect, g_DepthImage.View);
}

void RenderCore::AllocateEmptyTexture(VkFormat const TextureFormat)
{
    constexpr std::uint32_t                                DefaultTextureHalfSize { 2U };
    constexpr std::uint32_t                                DefaultTextureSize { DefaultTextureHalfSize * DefaultTextureHalfSize };
    constexpr std::array<std::uint8_t, DefaultTextureSize> DefaultTextureData {};

    VkCommandPool                CommandPool { VK_NULL_HANDLE };
    std::vector<VkCommandBuffer> CommandBuffers(1U, VK_NULL_HANDLE);

    auto const &[FamilyIndex, Queue] = GetGraphicsQueue();

    InitializeSingleCommandQueue(CommandPool, CommandBuffers, FamilyIndex);
    auto const [Index, Buffer, Allocation] = AllocateTexture(CommandBuffers.at(0U),
                                                             std::data(DefaultTextureData),
                                                             DefaultTextureHalfSize,
                                                             DefaultTextureHalfSize,
                                                             TextureFormat,
                                                             DefaultTextureSize * DefaultTextureSize);

    FinishSingleCommandQueue(Queue, CommandPool, CommandBuffers);

    VmaAllocator const &Allocator = GetAllocator();
    vmaDestroyBuffer(Allocator, Buffer, Allocation);
}

void RenderCore::LoadScene(strzilla::string_view const ModelPath)
{
    tinygltf::Model Model {};
    {
        tinygltf::TinyGLTF          ModelLoader {};
        std::string                 Error {};
        std::string                 Warning {};
        std::filesystem::path const ModelFilepath { std::data(ModelPath) };
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

    VkCommandPool                CommandPool { VK_NULL_HANDLE };
    std::vector<VkCommandBuffer> CommandBuffers { VK_NULL_HANDLE };

    auto const &                                                [QueueIndex, Queue] = GetGraphicsQueue();
    std::unordered_map<VkBuffer, VmaAllocation>                 BufferAllocations {};
    std::unordered_map<std::uint32_t, std::shared_ptr<Texture>> TextureMap {};

    InitializeSingleCommandQueue(CommandPool, CommandBuffers, QueueIndex);
    {
        VkCommandBuffer &CommandBuffer = CommandBuffers.at(0U);

        for (std::uint32_t Iterator = 0U; Iterator < std::size(Model.textures); ++Iterator)
        {
            tinygltf::Texture const &TextureIter = Model.textures.at(Iterator);
            if (TextureIter.source < 0 || TextureMap.contains(Iterator))
            {
                continue;
            }

            TextureConstructionInputParameters Input {
                    .ID = FetchID(),
                    .Image = Model.images.at(TextureIter.source),
                    .AllocationCmdBuffer = CommandBuffer
            };

            TextureConstructionOutputParameters Output {};

            if (std::shared_ptr<Texture> NewTexture = ConstructTexture(Input, Output);
                NewTexture)
            {
                TextureMap.emplace(Iterator, std::move(NewTexture));
                BufferAllocations.emplace(std::move(Output.StagingBuffer), std::move(Output.StagingAllocation));
            }
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
                MeshConstructionInputParameters Arguments {
                        .ID = FetchID(),
                        .Path = ModelPath,
                        .Model = Model,
                        .Node = Node,
                        .Mesh = LoadedMesh,
                        .Primitive = PrimitiveIter,
                        .TextureMap = TextureMap
                };

                if (std::shared_ptr<Mesh> NewMesh = ConstructMesh(Arguments);
                    NewMesh)
                {
                    std::uint32_t const ObjectID  = FetchID();
                    auto                NewObject = std::make_shared<Object>(ObjectID, ModelPath);

                    NewObject->SetMesh(std::move(NewMesh));
                    g_Objects.push_back(std::move(NewObject));
                }
            }
        }

        AllocateModelsBuffers(g_Objects);
    }
    FinishSingleCommandQueue(Queue, CommandPool, CommandBuffers);

    VmaAllocator const &Allocator = GetAllocator();
    for (auto &[Buffer, Allocation] : BufferAllocations)
    {
        vmaDestroyBuffer(Allocator, Buffer, Allocation);
    }
}

void RenderCore::UnloadObjects(std::vector<std::uint32_t> const &ObjectIDs)
{
    std::lock_guard Lock { g_ObjectMutex };

    std::for_each(std::execution::unseq,
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
    VkDevice const &LogicalDevice = GetLogicalDevice();

    if (g_Sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(LogicalDevice, g_Sampler, nullptr);
        g_Sampler = VK_NULL_HANDLE;
    }

    VmaAllocator const &Allocator = GetAllocator();
    m_UniformBufferAllocation.first.DestroyResources(Allocator);
    g_DepthImage.DestroyResources(Allocator);

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

    std::for_each(std::execution::unseq,
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

void RenderCore::UpdateSceneUniformBuffer()
{
    if (g_Camera.IsRenderDirty() || g_Illumination.IsRenderDirty())
    {
        constexpr auto SceneUBOSize = sizeof(SceneUniformData);

        SceneUniformData const UpdatedUBO {
                .LightPosition = g_Illumination.GetPosition(),
                .LightColor = g_Illumination.GetColor() * g_Illumination.GetIntensity(),
                .LightDiffuse = {},
                .LightAmbient = {},
                .LightSpecular = {}
        };

        std::memcpy(m_UniformBufferAllocation.first.MappedData, &UpdatedUBO, SceneUBOSize);
    }
}

void RenderCore::UpdateObjectsUniformBuffer()
{
    std::for_each(std::execution::unseq,
                  std::begin(g_Objects),
                  std::end(g_Objects),
                  [](std::shared_ptr<Object> const &ObjectIter)
                  {
                      ObjectIter->UpdateUniformBuffers();
                  });
}