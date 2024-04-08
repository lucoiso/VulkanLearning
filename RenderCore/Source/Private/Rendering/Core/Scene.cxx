// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <Volk/volk.h>
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
#include <execution>
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
import RenderCore.Utils.Constants;

using namespace RenderCore;

Camera       g_Camera {};
Illumination g_Illumination {};

VkSampler           g_Sampler { VK_NULL_HANDLE };
ImageAllocation     g_DepthImage {};
ImageAllocation     g_EmptyImage {};
std::atomic         g_ObjectIDCounter { 0U };
std::vector<Object> g_Objects {};
std::mutex          g_ObjectMutex {};

void RenderCore::CreateSceneUniformBuffers()
{
    g_Camera.AllocateUniformBuffer();
    g_Illumination.AllocateUniformBuffer();
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
    const auto [Buffer, Allocation] = AllocateTexture(CommandBuffers.at(0U),
                                                      std::data(DefaultTextureData),
                                                      DefaultTextureHalfSize,
                                                      DefaultTextureHalfSize,
                                                      TextureFormat,
                                                      DefaultTextureSize * DefaultTextureSize,
                                                      g_EmptyImage);
    FinishSingleCommandQueue(Queue, CopyCommandPool, CommandBuffers);

    vmaDestroyBuffer(GetAllocator(), Buffer, Allocation);
}

void RenderCore::LoadObject(std::string_view const ModelPath)
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

    auto const &                                [QueueIndex, Queue] = GetGraphicsQueue();
    std::unordered_map<VkBuffer, VmaAllocation> BufferAllocations {};

    InitializeSingleCommandQueue(CopyCommandPool, CommandBuffers, QueueIndex);
    {
        VkCommandBuffer &                                              CommandBuffer = CommandBuffers.at(0U);
        std::unordered_map<std::uint32_t, tinygltf::Primitive const &> CachedPrimitiveRelationship {};
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

            for (tinygltf::Primitive const &PrimitiveIter : Mesh.primitives)
            {
                std::uint32_t const ObjectID = g_ObjectIDCounter.fetch_add(1U);
                CachedPrimitiveRelationship.emplace(ObjectID, PrimitiveIter);

                Object NewObject(ObjectID, ModelPath, std::format("{}_{:03d}", std::empty(Mesh.name) ? "None" : Mesh.name, ObjectID));

                SetVertexAttributes(NewObject, Model, PrimitiveIter);
                SetPrimitiveTransform(NewObject, Node);
                AllocatePrimitiveIndices(NewObject, Model, PrimitiveIter);
                AllocateModelBuffers(NewObject);
                BufferAllocations.merge(AllocateObjectMaterials(CommandBuffer, NewObject, CachedPrimitiveRelationship.at(ObjectID), Model));

                g_Objects.push_back(std::move(NewObject));
            }
        }
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
                                                                         [ObjectIDIter](Object const &ObjectIter)
                                                                         {
                                                                             return ObjectIter.GetID() == ObjectIDIter;
                                                                         });
                          MatchingIter != std::end(g_Objects))
                      {
                          MatchingIter->Destroy();
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
    g_Camera.Destroy();
    g_Illumination.Destroy();
    DestroyObjects();
}

void RenderCore::DestroyObjects()
{
    std::lock_guard Lock { g_ObjectMutex };

    for (Object &Object : g_Objects)
    {
        Object.Destroy();
    }

    g_Objects.clear();
    g_ObjectIDCounter = 0U;
}

void RenderCore::TickObjects(float const DeltaTime)
{
    std::lock_guard Lock { g_ObjectMutex };

    std::erase_if(g_Objects,
                  [](Object const &ObjectIter)
                  {
                      return ObjectIter.IsPendingDestroy();
                  });

    std::for_each(std::execution::par_unseq,
                  std::begin(g_Objects),
                  std::end(g_Objects),
                  [&, DeltaTime](Object &ObjectIter)
                  {
                      if (!ObjectIter.IsPendingDestroy())
                      {
                          ObjectIter.Tick(DeltaTime);
                      }
                  });
}

std::lock_guard<std::mutex> RenderCore::LockScene()
{
    return std::lock_guard { g_ObjectMutex };
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

std::vector<Object> &RenderCore::GetObjects()
{
    return g_Objects;
}

std::uint32_t RenderCore::GetNumAllocations()
{
    return static_cast<std::uint32_t>(std::size(g_Objects));
}

void RenderCore::UpdateSceneUniformBuffers()
{
    g_Camera.UpdateUniformBuffers();
    g_Illumination.UpdateUniformBuffers();
}

Camera &RenderCore::GetCamera()
{
    return g_Camera;
}

Illumination &RenderCore::GetIllumination()
{
    return g_Illumination;
}
