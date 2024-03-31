// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <Volk/volk.h>
#include <algorithm>
#include <boost/log/trivial.hpp>
#include <filesystem>
#include <glm/ext.hpp>
#include <ranges>
#include "Utils/Library/Macros.h"

#ifndef VMA_IMPLEMENTATION
    #define VMA_IMPLEMENTATION
#endif
#include <vma/vk_mem_alloc.h>

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

#ifdef GLFW_INCLUDE_VULKAN
    #undef GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

module RenderCore.Runtime.Buffer;

import RuntimeInfo.Manager;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Types.TextureType;
import RenderCore.Runtime.Device;
import RenderCore.Runtime.Command;
import RenderCore.Runtime.Buffer.Operations;
import RenderCore.Runtime.Buffer.ModelAllocation;
import RenderCore.Subsystem.Allocation;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;

using namespace RenderCore;

VkSurfaceKHR   g_Surface {VK_NULL_HANDLE};
VmaAllocator   g_Allocator {VK_NULL_HANDLE};
VkSwapchainKHR g_SwapChain {VK_NULL_HANDLE};
VkSwapchainKHR g_OldSwapChain {VK_NULL_HANDLE};
VkFormat       g_SwapChainImageFormat {VK_FORMAT_UNDEFINED};
VkExtent2D     g_SwapChainExtent {0U, 0U};

std::vector<ImageAllocation> g_SwapChainImages {};

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
std::vector<ImageAllocation> g_ViewportImages {};
#endif

VkSampler                                           g_Sampler {VK_NULL_HANDLE};
ImageAllocation                                     g_DepthImage {};
ImageAllocation                                     g_EmptyImage {};
VkFormat                                            g_DepthFormat {VK_FORMAT_UNDEFINED};
std::vector<ObjectData>                             g_Objects {};
std::atomic                                         g_ObjectIDCounter {0U};
std::pair<BufferAllocation, VkDescriptorBufferInfo> g_SceneUniformBufferAllocation {};

void RenderCore::CreateSceneUniformBuffers()
{
    PUSH_CALLSTACK();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating Vulkan scene uniform buffers";

    constexpr VkDeviceSize BufferSize = sizeof(SceneUniformData);

    CreateUniformBuffers(g_Allocator, g_SceneUniformBufferAllocation.first, BufferSize, "SCENE_UNIFORM");
    g_SceneUniformBufferAllocation.second = {.buffer = g_SceneUniformBufferAllocation.first.Buffer, .offset = 0U, .range = BufferSize};
}

void RenderCore::CreateVulkanSurface(GLFWwindow *const Window)
{
    PUSH_CALLSTACK_WITH_COUNTER();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating vulkan surface";

    CheckVulkanResult(glfwCreateWindowSurface(volkGetLoadedInstance(), Window, nullptr, &g_Surface));
}

void RenderCore::CreateMemoryAllocator(VkPhysicalDevice const &PhysicalDevice)
{
    PUSH_CALLSTACK_WITH_COUNTER();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating vulkan memory allocator";

    VmaVulkanFunctions const VulkanFunctions {.vkGetInstanceProcAddr = vkGetInstanceProcAddr, .vkGetDeviceProcAddr = vkGetDeviceProcAddr};

    constexpr VmaDeviceMemoryCallbacks AllocationCallbacks {.pfnAllocate = AllocationSubsystem::AllocateDeviceMemoryCallback,
                                                            .pfnFree     = AllocationSubsystem::FreeDeviceMemoryCallback};

    VmaAllocatorCreateInfo const AllocatorInfo {.flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT | VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT,
                                                .physicalDevice                 = PhysicalDevice,
                                                .device                         = volkGetLoadedDevice(),
                                                .preferredLargeHeapBlockSize    = 0U /*Default: 256 MiB*/,
                                                .pAllocationCallbacks           = nullptr,
                                                .pDeviceMemoryCallbacks         = &AllocationCallbacks,
                                                .pHeapSizeLimit                 = nullptr,
                                                .pVulkanFunctions               = &VulkanFunctions,
                                                .instance                       = volkGetLoadedInstance(),
                                                .vulkanApiVersion               = VK_API_VERSION_1_3,
                                                .pTypeExternalMemoryHandleTypes = nullptr};

    CheckVulkanResult(vmaCreateAllocator(&AllocatorInfo, &g_Allocator));
}

void RenderCore::CreateImageSampler()
{
    PUSH_CALLSTACK_WITH_COUNTER();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating vulkan image sampler";

    CreateTextureSampler(g_Allocator->GetPhysicalDevice(), g_Sampler);
}

void RenderCore::CreateSwapChain(SurfaceProperties const &SurfaceProperties, VkSurfaceCapabilitiesKHR const &Capabilities)
{
    PUSH_CALLSTACK_WITH_COUNTER();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating Vulkan swap chain";

    auto const QueueFamilyIndices      = GetUniqueQueueFamilyIndicesU32();
    auto const QueueFamilyIndicesCount = static_cast<std::uint32_t>(std::size(QueueFamilyIndices));

    g_OldSwapChain         = g_SwapChain;
    g_SwapChainExtent      = SurfaceProperties.Extent;
    g_SwapChainImageFormat = SurfaceProperties.Format.format;

    VkSwapchainCreateInfoKHR const SwapChainCreateInfo {.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                                                        .surface          = GetSurface(),
                                                        .minImageCount    = g_MinImageCount,
                                                        .imageFormat      = g_SwapChainImageFormat,
                                                        .imageColorSpace  = SurfaceProperties.Format.colorSpace,
                                                        .imageExtent      = g_SwapChainExtent,
                                                        .imageArrayLayers = 1U,
                                                        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                                        .imageSharingMode
                                                        = QueueFamilyIndicesCount > 1U ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
                                                        .queueFamilyIndexCount = QueueFamilyIndicesCount,
                                                        .pQueueFamilyIndices   = std::data(QueueFamilyIndices),
                                                        .preTransform          = Capabilities.currentTransform,
                                                        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                                                        .presentMode           = SurfaceProperties.Mode,
                                                        .clipped               = VK_TRUE,
                                                        .oldSwapchain          = g_OldSwapChain};

    CheckVulkanResult(vkCreateSwapchainKHR(volkGetLoadedDevice(), &SwapChainCreateInfo, nullptr, &g_SwapChain));

    if (g_OldSwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(volkGetLoadedDevice(), g_OldSwapChain, nullptr);
        g_OldSwapChain = VK_NULL_HANDLE;
    }

    std::uint32_t Count = 0U;
    CheckVulkanResult(vkGetSwapchainImagesKHR(volkGetLoadedDevice(), g_SwapChain, &Count, nullptr));

    std::vector<VkImage> SwapChainImages(Count, VK_NULL_HANDLE);
    CheckVulkanResult(vkGetSwapchainImagesKHR(volkGetLoadedDevice(), g_SwapChain, &Count, std::data(SwapChainImages)));

    g_SwapChainImages.resize(Count, ImageAllocation());
    std::ranges::transform(SwapChainImages,
                           std::begin(g_SwapChainImages),
                           [](VkImage const &Image)
                           {
                               return ImageAllocation {.Image = Image};
                           });
    CreateSwapChainImageViews(g_SwapChainImages, SurfaceProperties.Format.format);
}

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
void RenderCore::CreateViewportResources(SurfaceProperties const &SurfaceProperties)
{
    PUSH_CALLSTACK_WITH_COUNTER();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating Vulkan viewport resources";

    std::ranges::for_each(g_ViewportImages,
                          [&](ImageAllocation &ImageIter)
                          {
                              ImageIter.DestroyResources(g_Allocator);
                          });
    g_ViewportImages.resize(std::size(g_SwapChainImages));

    constexpr VmaMemoryUsage     MemoryUsage = VMA_MEMORY_USAGE_AUTO;
    constexpr VkImageAspectFlags AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    constexpr VkImageTiling      Tiling      = VK_IMAGE_TILING_LINEAR;
    constexpr VkImageUsageFlags  UsageFlags
        = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    constexpr VmaAllocationCreateFlags MemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

    std::ranges::for_each(g_ViewportImages,
                          [&](ImageAllocation &ImageIter)
                          {
                              CreateImage(g_Allocator,
                                          SurfaceProperties.Format.format,
                                          SurfaceProperties.Extent,
                                          Tiling,
                                          UsageFlags,
                                          MemoryPropertyFlags,
                                          MemoryUsage,
                                          "VIEWPORT_IMAGE",
                                          ImageIter.Image,
                                          ImageIter.Allocation);

                              CreateImageView(ImageIter.Image, SurfaceProperties.Format.format, AspectFlags, ImageIter.View);
                          });
}
#endif

void RenderCore::CreateDepthResources(SurfaceProperties const &SurfaceProperties)
{
    PUSH_CALLSTACK_WITH_COUNTER();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating vulkan depth resources";

    if (g_DepthImage.IsValid())
    {
        g_DepthImage.DestroyResources(g_Allocator);
    }

    constexpr VmaMemoryUsage           MemoryUsage         = VMA_MEMORY_USAGE_AUTO;
    constexpr VkImageTiling            Tiling              = VK_IMAGE_TILING_OPTIMAL;
    constexpr VkImageUsageFlagBits     Usage               = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    constexpr VmaAllocationCreateFlags MemoryPropertyFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    constexpr VkImageAspectFlags       Aspect              = VK_IMAGE_ASPECT_DEPTH_BIT;

    g_DepthFormat = SurfaceProperties.DepthFormat;

    CreateImage(g_Allocator,
                g_DepthFormat,
                SurfaceProperties.Extent,
                Tiling,
                Usage,
                MemoryPropertyFlags,
                MemoryUsage,
                "DEPTH",
                g_DepthImage.Image,
                g_DepthImage.Allocation);

    CreateImageView(g_DepthImage.Image, g_DepthFormat, DepthHasStencil(g_DepthFormat) ? Aspect | VK_IMAGE_ASPECT_STENCIL_BIT : Aspect, g_DepthImage.View);
}

void RenderCore::AllocateEmptyTexture(VkFormat const TextureFormat)
{
    PUSH_CALLSTACK();

    constexpr std::uint8_t                                 DefaultTextureHalfSize {2U};
    constexpr std::uint8_t                                 DefaultTextureSize {DefaultTextureHalfSize * 2U};
    constexpr std::array<std::uint8_t, DefaultTextureSize> DefaultTextureData {};

    ImageCreationData CreationData
        = AllocateTexture(g_Allocator, std::data(DefaultTextureData), DefaultTextureHalfSize, DefaultTextureHalfSize, TextureFormat, DefaultTextureSize);

    VkCommandPool                CopyCommandPool {VK_NULL_HANDLE};
    std::vector<VkCommandBuffer> CommandBuffers(1U, VK_NULL_HANDLE);

    auto const &[FamilyIndex, Queue] = GetGraphicsQueue();

    InitializeSingleCommandQueue(CopyCommandPool, CommandBuffers, FamilyIndex);
    {
        MoveImageLayout<VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT>(CommandBuffers.at(0U),
                                                                                                                    CreationData.Allocation.Image,
                                                                                                                    CreationData.Format);

        CopyBufferToImage(CommandBuffers.at(0U), CreationData.StagingBuffer.first, CreationData.Allocation.Image, CreationData.Extent);

        MoveImageLayout<VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR, VK_IMAGE_ASPECT_COLOR_BIT>(CommandBuffers.at(0U),
                                                                                                                                CreationData.Allocation.Image,
                                                                                                                                CreationData.Format);
    }
    FinishSingleCommandQueue(Queue, CopyCommandPool, CommandBuffers);

    CreateImageView(CreationData.Allocation.Image, CreationData.Format, VK_IMAGE_ASPECT_COLOR_BIT, CreationData.Allocation.View);

    vmaDestroyBuffer(g_Allocator, CreationData.StagingBuffer.first, CreationData.StagingBuffer.second);
    g_EmptyImage = std::move(CreationData.Allocation);
}

std::vector<Object> RenderCore::PrepareSceneAllocationResources(std::vector<ObjectData> &AllocationData)
{
    PUSH_CALLSTACK();

    g_Objects.reserve(std::size(g_Objects) + std::size(AllocationData));

    std::vector<Object> Output;
    Output.reserve(std::size(AllocationData));

    std::vector<BufferCopyOperationData> BufferCopyOperationDatas {};
    BufferCopyOperationDatas.reserve(std::size(AllocationData));

    std::vector<MoveOperationData> MoveOperation {};
    std::vector<CopyOperationData> CopyOperation {};

    for (ObjectData const &ObjectIter : AllocationData)
    {
        auto &[VertexData, IndexData] = BufferCopyOperationDatas.emplace_back();

        VertexData.SourceBuffer          = ObjectIter.CommandBufferSets.at(0U).StagingBuffer.first;
        VertexData.SourceAllocation      = ObjectIter.CommandBufferSets.at(0U).StagingBuffer.second;
        VertexData.DestinationBuffer     = ObjectIter.Allocation.VertexBufferAllocation.Buffer;
        VertexData.DestinationAllocation = ObjectIter.Allocation.VertexBufferAllocation.Allocation;
        VertexData.AllocationSize        = ObjectIter.CommandBufferSets.at(0U).AllocationSize;

        IndexData.SourceBuffer          = ObjectIter.CommandBufferSets.at(1U).StagingBuffer.first;
        IndexData.SourceAllocation      = ObjectIter.CommandBufferSets.at(1U).StagingBuffer.second;
        IndexData.DestinationBuffer     = ObjectIter.Allocation.IndexBufferAllocation.Buffer;
        IndexData.DestinationAllocation = ObjectIter.Allocation.IndexBufferAllocation.Allocation;
        IndexData.AllocationSize        = ObjectIter.CommandBufferSets.at(1U).AllocationSize;

        MoveOperation.reserve(std::size(MoveOperation) + std::size(ObjectIter.ImageCreationDatas));
        for (ImageCreationData const &ImageCreationDataIter : ObjectIter.ImageCreationDatas)
        {
            MoveOperation.push_back({.Image = ImageCreationDataIter.Allocation.Image, .Format = ImageCreationDataIter.Format});

            CopyOperation.push_back({.SourceBuffer     = ImageCreationDataIter.StagingBuffer.first,
                                     .SourceAllocation = ImageCreationDataIter.StagingBuffer.second,
                                     .DestinationImage = ImageCreationDataIter.Allocation.Image,
                                     .Format           = ImageCreationDataIter.Format,
                                     .Extent           = ImageCreationDataIter.Extent});
        }

        Output.push_back(ObjectIter.Object);
    }

    auto const &[FamilyIndex, Queue] = GetGraphicsQueue();

    VkCommandPool                CopyCommandPool {VK_NULL_HANDLE};
    std::vector<VkCommandBuffer> CommandBuffers {};
    CommandBuffers.resize(std::size(AllocationData) + std::size(MoveOperation) + std::size(CopyOperation) + std::size(MoveOperation));

    InitializeSingleCommandQueue(CopyCommandPool, CommandBuffers, FamilyIndex);
    {
        std::uint32_t Count = 0U;

        for (auto const &[VertexData, IndexData] : BufferCopyOperationDatas)
        {
            VkCommandBuffer const &CommandBuffer = CommandBuffers.at(Count);

            VkBufferCopy const VertexBufferCopy {.size = VertexData.AllocationSize};
            vkCmdCopyBuffer(CommandBuffer, VertexData.SourceBuffer, VertexData.DestinationBuffer, 1U, &VertexBufferCopy);

            VkBufferCopy const IndexBufferCopy {.size = IndexData.AllocationSize};
            vkCmdCopyBuffer(CommandBuffer, IndexData.SourceBuffer, IndexData.DestinationBuffer, 1U, &IndexBufferCopy);

            ++Count;
        }

        for (auto const &[Image, Format] : MoveOperation)
        {
            MoveImageLayout<VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT>(CommandBuffers.at(Count),
                                                                                                                        Image,
                                                                                                                        Format);
            ++Count;
        }

        for (auto const &[SourceBuffer, SourceAllocation, DestinationImage, Format, Extent] : CopyOperation)
        {
            CopyBufferToImage(CommandBuffers.at(Count), SourceBuffer, DestinationImage, Extent);
            ++Count;
        }

        for (auto const &[Image, Format] : MoveOperation)
        {
            MoveImageLayout<VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR, VK_IMAGE_ASPECT_COLOR_BIT>(CommandBuffers.at(Count),
                                                                                                                                    Image,
                                                                                                                                    Format);
            ++Count;
        }
    }
    FinishSingleCommandQueue(Queue, CopyCommandPool, CommandBuffers);

    for (ObjectData &ObjectIter : AllocationData)
    {
        for (ImageCreationData &ImageCreationDataIter : ObjectIter.ImageCreationDatas)
        {
            CreateTextureImageView(ImageCreationDataIter.Allocation, GetSwapChainImageFormat());

            ObjectIter.Allocation.TextureImageAllocations.push_back(ImageCreationDataIter.Allocation);
            ObjectIter.Allocation.TextureDescriptors.emplace(ImageCreationDataIter.Type,
                                                             VkDescriptorImageInfo {.sampler     = g_Sampler,
                                                                                    .imageView   = ImageCreationDataIter.Allocation.View,
                                                                                    .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR});

            vmaDestroyBuffer(g_Allocator, ImageCreationDataIter.StagingBuffer.first, ImageCreationDataIter.StagingBuffer.second);
        }
        ObjectIter.ImageCreationDatas.clear();

        for (std::uint8_t TextTypeIter = 0U; TextTypeIter <= static_cast<std::uint8_t>(TextureType::MetallicRoughness); ++TextTypeIter)
        {
            if (!ObjectIter.Allocation.TextureDescriptors.contains(static_cast<TextureType>(TextTypeIter)))
            {
                ObjectIter.Allocation.TextureDescriptors.emplace(
                    static_cast<TextureType>(TextTypeIter),
                    VkDescriptorImageInfo {.sampler = g_Sampler, .imageView = g_EmptyImage.View, .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR});
            }
        }

        for (CommandBufferSet &CommandBufferSetIter : ObjectIter.CommandBufferSets)
        {
            vmaDestroyBuffer(g_Allocator, CommandBufferSetIter.StagingBuffer.first, CommandBufferSetIter.StagingBuffer.second);
        }
        ObjectIter.CommandBufferSets.clear();
    }

    std::ranges::move(AllocationData, std::back_inserter(g_Objects));

    return Output;
}

std::vector<Object> RenderCore::AllocateScene(std::string_view const ModelPath)
{
    PUSH_CALLSTACK();

    std::vector<ObjectData> AllocationData {};
    tinygltf::Model         Model {};
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
            return {};
        }
    }

    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Loaded model from path: '" << ModelPath << "'";
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Loading scenes: " << std::size(Model.scenes);

    for (tinygltf::Node const &Node : Model.nodes)
    {
        std::int32_t const MeshIndex = Node.mesh;
        if (MeshIndex < 0)
        {
            continue;
        }

        tinygltf::Mesh const &Mesh = Model.meshes.at(MeshIndex);
        AllocationData.reserve(std::size(AllocationData) + std::size(Mesh.primitives));

        for (tinygltf::Primitive const &Primitive : Mesh.primitives)
        {
            std::uint32_t const ObjectID = g_ObjectIDCounter.fetch_add(1U);

            ObjectData &NewObjectAllocation
                = AllocationData.emplace_back(ObjectData {.Object {ObjectID, ModelPath, std::format("{}_{:03d}", Mesh.name, ObjectID)}});

            AllocateVertexAttributes(NewObjectAllocation, Model, Primitive);
            NewObjectAllocation.Object.SetTrianglesCount(AllocatePrimitiveIndices(NewObjectAllocation, Model, Primitive));
            SetPrimitiveTransform(NewObjectAllocation.Object, Node);
            AllocatePrimitiveMaterials(NewObjectAllocation, Model, Primitive, g_Allocator, GetSwapChainImageFormat());

            NewObjectAllocation.CommandBufferSets.push_back(CreateVertexBuffers(g_Allocator, NewObjectAllocation.Allocation, NewObjectAllocation.Vertices));
            NewObjectAllocation.CommandBufferSets.push_back(CreateIndexBuffers(g_Allocator, NewObjectAllocation.Allocation, NewObjectAllocation.Indices));
            CreateModelUniformBuffers(g_Allocator, NewObjectAllocation.Allocation);
        }
    }

    return PrepareSceneAllocationResources(AllocationData);
}

void RenderCore::ReleaseScene(std::vector<std::uint32_t> const &ObjectIDs)
{
    PUSH_CALLSTACK_WITH_COUNTER();

    std::ranges::for_each(ObjectIDs,
                          [&](std::uint32_t const ObjectIDIter)
                          {
                              if (auto const MatchingIter = std::ranges::find_if(g_Objects,
                                                                                 [ObjectIDIter](ObjectData const &ObjectIter)
                                                                                 {
                                                                                     return ObjectIter.Object.GetID() == ObjectIDIter;
                                                                                 });
                                  MatchingIter != std::end(g_Objects))
                              {
                                  MatchingIter->Allocation.DestroyResources(g_Allocator);
                                  g_Objects.erase(MatchingIter);
                              }
                          });

    if (std::empty(g_Objects))
    {
        g_ObjectIDCounter = 0U;
    }
}

void RenderCore::ReleaseBufferResources()
{
    PUSH_CALLSTACK_WITH_COUNTER();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Releasing vulkan buffer resources";

    if (g_SwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(volkGetLoadedDevice(), g_SwapChain, nullptr);
        g_SwapChain = VK_NULL_HANDLE;
    }

    if (g_OldSwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(volkGetLoadedDevice(), g_OldSwapChain, nullptr);
        g_OldSwapChain = VK_NULL_HANDLE;
    }

    if (g_Sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(volkGetLoadedDevice(), g_Sampler, nullptr);
        g_Sampler = VK_NULL_HANDLE;
    }

    if (g_EmptyImage.IsValid())
    {
        g_EmptyImage.DestroyResources(g_Allocator);
    }

    DestroyBufferResources(true);

    if (g_SceneUniformBufferAllocation.first.IsValid())
    {
        g_SceneUniformBufferAllocation.first.DestroyResources(g_Allocator);
    }

    vkDestroySurfaceKHR(volkGetLoadedInstance(), g_Surface, nullptr);
    g_Surface = VK_NULL_HANDLE;

    vmaDestroyAllocator(g_Allocator);
    g_Allocator = VK_NULL_HANDLE;
}

void RenderCore::DestroyBufferResources(bool const ClearScene)
{
    PUSH_CALLSTACK_WITH_COUNTER();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Destroying vulkan buffer resources";

    std::ranges::for_each(g_SwapChainImages,
                          [&](ImageAllocation &ImageIter)
                          {
                              ImageIter.DestroyResources(g_Allocator);
                          });
    g_SwapChainImages.clear();

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
    std::ranges::for_each(g_ViewportImages,
                          [&](ImageAllocation &ImageIter)
                          {
                              ImageIter.DestroyResources(g_Allocator);
                          });
    g_ViewportImages.clear();
#endif

    g_DepthImage.DestroyResources(g_Allocator);

    if (ClearScene)
    {
        for (auto &[Object, Allocation, Vertices, Indices, ImageCreationDatas, CommandBufferSets] : g_Objects)
        {
            Allocation.DestroyResources(g_Allocator);
        }
        g_Objects.clear();
    }
}

VkSurfaceKHR const &RenderCore::GetSurface()
{
    return g_Surface;
}

VkSwapchainKHR const &RenderCore::GetSwapChain()
{
    return g_SwapChain;
}

VkExtent2D const &RenderCore::GetSwapChainExtent()
{
    return g_SwapChainExtent;
}

VkFormat const &RenderCore::GetSwapChainImageFormat()
{
    return g_SwapChainImageFormat;
}

std::vector<ImageAllocation> const &RenderCore::GetSwapChainImages()
{
    return g_SwapChainImages;
}

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
std::vector<ImageAllocation> const &RenderCore::GetViewportImages()
{
    return g_ViewportImages;
}
#endif

ImageAllocation const &RenderCore::GetDepthImage()
{
    return g_DepthImage;
}

VkFormat const &RenderCore::GetDepthFormat()
{
    return g_DepthFormat;
}

VkSampler const &RenderCore::GetSampler()
{
    return g_Sampler;
}

VkBuffer RenderCore::GetVertexBuffer(std::uint32_t const ObjectID)
{
    if (auto const MatchingIter = std::ranges::find_if(g_Objects,
                                                       [ObjectID](ObjectData const &ObjectIter)
                                                       {
                                                           return ObjectIter.Object.GetID() == ObjectID;
                                                       });
        MatchingIter != std::end(g_Objects))
    {
        return MatchingIter->Allocation.VertexBufferAllocation.Buffer;
    }

    return VK_NULL_HANDLE;
}

VkBuffer RenderCore::GetIndexBuffer(std::uint32_t const ObjectID)
{
    if (auto const MatchingIter = std::ranges::find_if(g_Objects,
                                                       [ObjectID](ObjectData const &ObjectIter)
                                                       {
                                                           return ObjectIter.Object.GetID() == ObjectID;
                                                       });
        MatchingIter != std::end(g_Objects))
    {
        return MatchingIter->Allocation.IndexBufferAllocation.Buffer;
    }

    return VK_NULL_HANDLE;
}

std::uint32_t RenderCore::GetIndicesCount(std::uint32_t const ObjectID)
{
    if (auto const MatchingIter = std::ranges::find_if(g_Objects,
                                                       [ObjectID](ObjectData const &ObjectIter)
                                                       {
                                                           return ObjectIter.Object.GetID() == ObjectID;
                                                       });
        MatchingIter != std::end(g_Objects))
    {
        return static_cast<std::uint32_t>(std::size(MatchingIter->Indices));
    }

    return 0U;
}

void *RenderCore::GetSceneUniformData()
{
    return g_SceneUniformBufferAllocation.first.MappedData;
}

VkDescriptorBufferInfo const &RenderCore::GetSceneUniformDescriptor()
{
    return g_SceneUniformBufferAllocation.second;
}

void *RenderCore::GetModelUniformData(std::uint32_t const ObjectID)
{
    if (auto const MatchingIter = std::ranges::find_if(g_Objects,
                                                       [ObjectID](ObjectData const &ObjectIter)
                                                       {
                                                           return ObjectIter.Object.GetID() == ObjectID;
                                                       });
        MatchingIter != std::end(g_Objects))
    {
        return MatchingIter->Allocation.UniformBufferAllocation.MappedData;
    }

    return nullptr;
}

bool RenderCore::ContainsObject(std::uint32_t const ID)
{
    return std::ranges::find_if(g_Objects,
                                [ID](auto const &Object)
                                {
                                    return Object.Object.GetID() == ID;
                                })
        != std::end(g_Objects);
}

std::vector<ObjectData> const &RenderCore::GetAllocatedObjects()
{
    return g_Objects;
}

std::uint32_t RenderCore::GetNumAllocations()
{
    return static_cast<std::uint32_t>(std::size(g_Objects));
}

std::uint32_t RenderCore::GetClampedNumAllocations()
{
    return std::clamp(static_cast<std::uint32_t>(std::size(g_Objects)), 1U, UINT32_MAX);
}

void RenderCore::UpdateSceneUniformBuffers(Camera const &Camera, Illumination const &Illumination)
{
    PUSH_CALLSTACK();

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

void RenderCore::UpdateModelUniformBuffers(std::shared_ptr<Object> const &Object)
{
    PUSH_CALLSTACK();

    if (!Object)
    {
        return;
    }

    if (void *UniformBufferData = GetModelUniformData(Object->GetID()))
    {
        glm::mat4 const UpdatedUBO {Object->GetMatrix()};
        std::memcpy(UniformBufferData, &UpdatedUBO, sizeof(glm::mat4));
    }
}

void RenderCore::SaveImageToFile(VkImage const &Image, std::string_view const Path)
{
    PUSH_CALLSTACK();

    VkBuffer            Buffer;
    VmaAllocation       Allocation;
    VkSubresourceLayout Layout;

    std::uint32_t const    ImageWidth {g_SwapChainExtent.width};
    std::uint32_t const    ImageHeight {g_SwapChainExtent.height};
    constexpr std::uint8_t Components {4U};

    auto const &[FamilyIndex, Queue] = GetGraphicsQueue();

    VkCommandPool                CommandPool {VK_NULL_HANDLE};
    std::vector<VkCommandBuffer> CommandBuffer {VK_NULL_HANDLE};
    InitializeSingleCommandQueue(CommandPool, CommandBuffer, FamilyIndex);
    {
        VkImageSubresource SubResource {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .arrayLayer = 0};

        vkGetImageSubresourceLayout(volkGetLoadedDevice(), Image, &SubResource, &Layout);

        VkBufferCreateInfo BufferInfo {.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                       .size        = Layout.size,
                                       .usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                       .sharingMode = VK_SHARING_MODE_EXCLUSIVE};

        VmaAllocationCreateInfo AllocationInfo {.usage = VMA_MEMORY_USAGE_CPU_ONLY};

        vmaCreateBuffer(g_Allocator, &BufferInfo, &AllocationInfo, &Buffer, &Allocation, nullptr);

        VkBufferImageCopy Region {.bufferOffset      = 0U,
                                  .bufferRowLength   = 0U,
                                  .bufferImageHeight = 0U,
                                  .imageSubresource  = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0U, .baseArrayLayer = 0U, .layerCount = 1U},
                                  .imageOffset       = {0U, 0U, 0U},
                                  .imageExtent       = {.width = ImageWidth, .height = ImageHeight, .depth = 1U}};

        VkImageMemoryBarrier2 PreCopyBarrier {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR,
            .pNext               = nullptr,
            .srcStageMask        = VK_PIPELINE_STAGE_2_NONE_KHR,
            .srcAccessMask       = VK_ACCESS_2_NONE_KHR,
            .dstStageMask        = VK_PIPELINE_STAGE_TRANSFER_BIT,
            .dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT,
            .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = Image,
            .subresourceRange    = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0U, .levelCount = 1U, .baseArrayLayer = 0U, .layerCount = 1U}};

        VkImageMemoryBarrier2 PostCopyBarrier {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR,
            .pNext               = nullptr,
            .srcStageMask        = VK_PIPELINE_STAGE_TRANSFER_BIT,
            .srcAccessMask       = VK_ACCESS_TRANSFER_READ_BIT,
            .dstStageMask        = VK_PIPELINE_STAGE_2_NONE_KHR,
            .dstAccessMask       = VK_ACCESS_2_NONE_KHR,
            .oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = Image,
            .subresourceRange    = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0U, .levelCount = 1U, .baseArrayLayer = 0U, .layerCount = 1U}};

        VkDependencyInfo DependencyInfo {.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
                                         .imageMemoryBarrierCount = 1U,
                                         .pImageMemoryBarriers    = &PreCopyBarrier};

        vkCmdPipelineBarrier2(CommandBuffer.back(), &DependencyInfo);
        vkCmdCopyImageToBuffer(CommandBuffer.back(), Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Buffer, 1U, &Region);

        DependencyInfo.pImageMemoryBarriers = &PostCopyBarrier;
        vkCmdPipelineBarrier2(CommandBuffer.back(), &DependencyInfo);
    }
    FinishSingleCommandQueue(Queue, CommandPool, CommandBuffer);

    void *ImageData;
    vmaMapMemory(g_Allocator, Allocation, &ImageData);

    auto ImagePixels = static_cast<unsigned char *>(ImageData);

    std::ranges::for_each(std::views::iota(0U, ImageWidth * ImageHeight),
                          [&](std::uint32_t const Iterator)
                          {
                              std::swap(ImagePixels[Iterator * Components], ImagePixels[Iterator * Components + 2U]);
                          });

    stbi_write_png(std::data(Path), ImageWidth, ImageHeight, Components, ImagePixels, ImageWidth * Components);

    vmaUnmapMemory(g_Allocator, Allocation);
    vmaDestroyBuffer(g_Allocator, Buffer, Allocation);
}

VmaAllocator const &RenderCore::GetAllocator()
{
    return g_Allocator;
}
