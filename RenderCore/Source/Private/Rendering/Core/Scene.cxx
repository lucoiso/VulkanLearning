// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <Volk/volk.h>
#include <glm/ext.hpp>
#include <vma/vk_mem_alloc.h>
#include <boost/log/trivial.hpp>

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

import RenderCore.Types.Object;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Types.SurfaceProperties;
import RenderCore.Runtime.Device;
import RenderCore.Runtime.Command;
import RenderCore.Runtime.Memory;
import RenderCore.Runtime.Model;
import RenderCore.Runtime.SwapChain;
import RenderCore.Utils.Helpers;

using namespace RenderCore;

VkSampler                                           g_Sampler {VK_NULL_HANDLE};
ImageAllocation                                     g_DepthImage {};
ImageAllocation                                     g_EmptyImage {};
std::atomic                                         g_ObjectIDCounter {0U};
std::vector<std::shared_ptr<Object>>                g_Objects {};
std::pair<BufferAllocation, VkDescriptorBufferInfo> g_SceneUniformBufferAllocation {};

void RenderCore::CreateSceneUniformBuffers()
{
    constexpr VkDeviceSize BufferSize = sizeof(SceneUniformData);

    CreateUniformBuffers(g_SceneUniformBufferAllocation.first, BufferSize, "SCENE_UNIFORM");
    g_SceneUniformBufferAllocation.second = {.buffer = g_SceneUniformBufferAllocation.first.Buffer, .offset = 0U, .range = BufferSize};
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

    constexpr VmaMemoryUsage           MemoryUsage         = VMA_MEMORY_USAGE_AUTO;
    constexpr VkImageTiling            Tiling              = VK_IMAGE_TILING_OPTIMAL;
    constexpr VkImageUsageFlagBits     Usage               = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    constexpr VmaAllocationCreateFlags MemoryPropertyFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    constexpr VkImageAspectFlags       Aspect              = VK_IMAGE_ASPECT_DEPTH_BIT;

    g_DepthImage.Format = SurfaceProperties.DepthFormat;

    CreateImage(g_DepthImage.Format,
                SurfaceProperties.Extent,
                Tiling,
                Usage,
                MemoryPropertyFlags,
                MemoryUsage,
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
    constexpr std::uint8_t                                 DefaultTextureHalfSize {2U};
    constexpr std::uint8_t                                 DefaultTextureSize {DefaultTextureHalfSize * 2U};
    constexpr std::array<std::uint8_t, DefaultTextureSize> DefaultTextureData {};

    VkCommandPool                CopyCommandPool {VK_NULL_HANDLE};
    std::vector<VkCommandBuffer> CommandBuffers(1U, VK_NULL_HANDLE);

    auto const &[FamilyIndex, Queue] = GetGraphicsQueue();

    InitializeSingleCommandQueue(CopyCommandPool, CommandBuffers, FamilyIndex);
    const auto [Buffer, Allocation] = AllocateTexture(CommandBuffers.at(0U),
                                                      std::data(DefaultTextureData),
                                                      DefaultTextureHalfSize,
                                                      DefaultTextureHalfSize,
                                                      TextureFormat,
                                                      DefaultTextureSize,
                                                      g_EmptyImage);
    FinishSingleCommandQueue(Queue, CopyCommandPool, CommandBuffers);

    vmaDestroyBuffer(GetAllocator(), Buffer, Allocation);
}

void RenderCore::AllocateScene(std::string_view const ModelPath)
{
    tinygltf::Model Model {};
    {
        tinygltf::TinyGLTF          ModelLoader {};
        std::string                 Error {};
        std::string                 Warning {};
        std::filesystem::path const ModelFilepath(ModelPath);
        bool const LoadResult = ModelFilepath.extension() == ".gltf" ? ModelLoader.LoadASCIIFromFile(&Model, &Error, &Warning, std::data(ModelPath))
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

    std::unordered_map<std::shared_ptr<Object>, tinygltf::Primitive const &> CachedPrimitiveRelationship {};

    for (tinygltf::Node const &Node : Model.nodes)
    {
        std::int32_t const MeshIndex = Node.mesh;
        if (MeshIndex < 0)
        {
            continue;
        }

        tinygltf::Mesh const &Mesh = Model.meshes.at(MeshIndex);

        g_Objects.reserve(std::size(g_Objects) + std::size(Mesh.primitives));
        CachedPrimitiveRelationship.reserve(std::size(CachedPrimitiveRelationship) + std::size(Mesh.primitives));

        for (tinygltf::Primitive const &Primitive : Mesh.primitives)
        {
            std::uint32_t const ObjectID = g_ObjectIDCounter.fetch_add(1U);

            std::shared_ptr<Object> &NewObject
                = g_Objects.emplace_back(std::make_shared<Object>(ObjectID, ModelPath, std::format("{}_{:03d}", Mesh.name, ObjectID)));

            SetVertexAttributes(NewObject, Model, Primitive);
            SetPrimitiveTransform(NewObject, Node);
            AllocatePrimitiveIndices(NewObject, Model, Primitive);

            CachedPrimitiveRelationship.emplace(NewObject, Primitive);
        }
    }

    VkCommandPool                CopyCommandPool {VK_NULL_HANDLE};
    std::vector<VkCommandBuffer> CommandBuffers {};
    CommandBuffers.resize(std::size(CachedPrimitiveRelationship) * 2U);

    auto const &[QueueIndex, Queue] = GetGraphicsQueue();
    std::unordered_map<VkBuffer, VmaAllocation> BufferAllocations {};

    InitializeSingleCommandQueue(CopyCommandPool, CommandBuffers, QueueIndex);
    {
        std::int16_t Count = -1;
        for (std::pair<std::shared_ptr<Object>, tinygltf::Primitive const &> ObjectIter : CachedPrimitiveRelationship)
        {
            BufferAllocations.merge(AllocateObjectBuffers(CommandBuffers.at(++Count), ObjectIter.first));
            BufferAllocations.merge(AllocateObjectMaterials(CommandBuffers.at(++Count), ObjectIter.first, ObjectIter.second, Model));
        }
    }
    FinishSingleCommandQueue(Queue, CopyCommandPool, CommandBuffers);

    for (auto &[Buffer, Allocation] : BufferAllocations)
    {
        vmaDestroyBuffer(GetAllocator(), Buffer, Allocation);
    }
}

void RenderCore::ReleaseScene(std::vector<std::uint32_t> const &ObjectIDs)
{
    std::ranges::for_each(ObjectIDs,
                          [&](std::uint32_t const ObjectIDIter)
                          {
                              if (auto const MatchingIter = std::ranges::find_if(g_Objects,
                                                                                 [ObjectIDIter](std::shared_ptr<Object> const &ObjectIter)
                                                                                 {
                                                                                     return ObjectIter->GetID() == ObjectIDIter;
                                                                                 });
                                  MatchingIter != std::end(g_Objects))
                              {
                                  (*MatchingIter)->Destroy();
                                  g_Objects.erase(MatchingIter);
                              }
                          });

    if (std::empty(g_Objects))
    {
        g_ObjectIDCounter = 0U;
    }
}

void RenderCore::ReleaseSceneResources()
{
    if (g_Sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(volkGetLoadedDevice(), g_Sampler, nullptr);
        g_Sampler = VK_NULL_HANDLE;
    }

    g_EmptyImage.DestroyResources(GetAllocator());
    g_DepthImage.DestroyResources(GetAllocator());
    g_SceneUniformBufferAllocation.first.DestroyResources(GetAllocator());
    DestroyObjects();
}

void RenderCore::DestroyObjects()
{
    for (std::shared_ptr<Object> const &ObjectIter : g_Objects)
    {
        ObjectIter->Destroy();
    }
    g_Objects.clear();
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

void *RenderCore::GetSceneUniformData()
{
    return g_SceneUniformBufferAllocation.first.MappedData;
}

VkDescriptorBufferInfo const &RenderCore::GetSceneUniformDescriptor()
{
    return g_SceneUniformBufferAllocation.second;
}

std::vector<std::shared_ptr<Object>> const &RenderCore::GetAllocatedObjects()
{
    return g_Objects;
}

std::uint32_t RenderCore::GetNumAllocations()
{
    return static_cast<std::uint32_t>(std::size(g_Objects));
}

void RenderCore::UpdateSceneUniformBuffers(Camera const &Camera, Illumination const &Illumination)
{
    if (void *UniformBufferData = GetSceneUniformData())
    {
        SceneUniformData const UpdatedUBO {
            .Projection    = Camera.GetProjectionMatrix(GetSwapChainExtent()),
            .View          = Camera.GetViewMatrix(),
            .LightPosition = Illumination.GetPosition().ToGlmVec4(),
            .LightColor    = Illumination.GetColor().ToGlmVec4() * Illumination.GetIntensity(),
        };

        std::memcpy(UniformBufferData, &UpdatedUBO, sizeof(SceneUniformData));
    }
}
