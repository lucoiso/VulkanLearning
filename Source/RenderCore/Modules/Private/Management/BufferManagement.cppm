// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <boost/log/trivial.hpp>
#include <glm/ext.hpp>
#include <volk.h>

#ifndef VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#endif
#include <vk_mem_alloc.h>

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

module RenderCore.Management.BufferManagement;

import <filesystem>;
import <span>;
import <algorithm>;
import <ranges>;

import RenderCore.EngineCore;
import RenderCore.Management.CommandsManagement;
import RenderCore.Management.DeviceManagement;
import RenderCore.Management.PipelineManagement;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Types.Vertex;
import RenderCore.Types.Camera;
import RenderCore.Types.Object;

using namespace RenderCore;

namespace Allocation
{
    struct ImageAllocation
    {
        VkImage Image {VK_NULL_HANDLE};
        VkImageView View {VK_NULL_HANDLE};
        VkSampler Sampler {VK_NULL_HANDLE};
        VmaAllocation Allocation {VK_NULL_HANDLE};

        TextureType Type {TextureType::BaseColor};

        [[nodiscard]] bool IsValid() const
        {
            return Image != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE;
        }

        void DestroyResources()
        {
            if (Image != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE)
            {
                vmaDestroyImage(GetAllocator(), Image, Allocation);
                Image      = VK_NULL_HANDLE;
                Allocation = VK_NULL_HANDLE;
            }

            if (View != VK_NULL_HANDLE)
            {
                vkDestroyImageView(volkGetLoadedDevice(), View, nullptr);
                View = VK_NULL_HANDLE;

                if (Image != VK_NULL_HANDLE)
                {
                    Image = VK_NULL_HANDLE;
                }
            }

            if (Sampler != VK_NULL_HANDLE)
            {
                vkDestroySampler(volkGetLoadedDevice(), Sampler, nullptr);
                Sampler = VK_NULL_HANDLE;
            }
        }
    };

    struct BufferAllocation
    {
        VkBuffer Buffer {VK_NULL_HANDLE};
        VmaAllocation Allocation {VK_NULL_HANDLE};
        void* MappedData {nullptr};

        [[nodiscard]] bool IsValid() const
        {
            return Buffer != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE;
        }

        void DestroyResources()
        {
            if (Buffer != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE)
            {
                if (MappedData)
                {
                    vmaUnmapMemory(GetAllocator(), Allocation);
                    MappedData = nullptr;
                }

                vmaDestroyBuffer(GetAllocator(), Buffer, Allocation);
                Allocation = VK_NULL_HANDLE;
                Buffer     = VK_NULL_HANDLE;
            }
        }
    };

    struct ObjectAllocation
    {
        std::vector<ImageAllocation> TextureImages {};
        BufferAllocation VertexBuffer {};
        BufferAllocation IndexBuffer {};
        BufferAllocation UniformBuffer {};
        std::uint32_t IndicesCount {0U};

        [[nodiscard]] bool IsValid() const
        {
            return VertexBuffer.IsValid() && IndexBuffer.IsValid() && IndicesCount != 0U;
        }

        void DestroyResources()
        {
            VertexBuffer.DestroyResources();
            IndexBuffer.DestroyResources();

            for (ImageAllocation& TextureImage: TextureImages)
            {
                TextureImage.DestroyResources();
            }

            UniformBuffer.DestroyResources();
            IndicesCount = 0U;
        }
    };
}// namespace Allocation

VkSurfaceKHR g_Surface {VK_NULL_HANDLE};
VmaAllocator g_Allocator {VK_NULL_HANDLE};
VkSwapchainKHR g_SwapChain {VK_NULL_HANDLE};
VkSwapchainKHR g_OldSwapChain {VK_NULL_HANDLE};
VkExtent2D g_SwapChainExtent {0U, 0U};
std::vector<Allocation::ImageAllocation> g_SwapChainImages {};
Allocation::ImageAllocation g_DepthImage {};
std::vector<VkFramebuffer> g_FrameBuffers {};
std::unordered_map<std::uint32_t, Allocation::ObjectAllocation> g_Objects {};
std::atomic g_ObjectIDCounter {0U};

VmaAllocationInfo CreateBuffer(VkDeviceSize const& Size, VkBufferUsageFlags const Usage, VkMemoryPropertyFlags const Flags, VkBuffer& Buffer, VmaAllocation& Allocation)
{
    if (g_Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    VmaAllocationCreateInfo const AllocationCreateInfo {
            .flags = Flags,
            .usage = VMA_MEMORY_USAGE_AUTO};

    VkBufferCreateInfo const BufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size  = std::clamp(Size, g_BufferMemoryAllocationSize, UINT64_MAX),
            .usage = Usage};

    VmaAllocationInfo MemoryAllocationInfo;
    CheckVulkanResult(vmaCreateBuffer(g_Allocator, &BufferCreateInfo, &AllocationCreateInfo, &Buffer, &Allocation, &MemoryAllocationInfo));

    return MemoryAllocationInfo;
}

void CopyBuffer(VkBuffer const& Source, VkBuffer const& Destination, VkDeviceSize const& Size, VkQueue const& Queue, std::uint8_t const QueueFamilyIndex)
{
    VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
    VkCommandPool CommandPool     = VK_NULL_HANDLE;
    InitializeSingleCommandQueue(CommandPool, CommandBuffer, QueueFamilyIndex);
    {
        VkBufferCopy const BufferCopy {
                .size = Size,
        };

        vkCmdCopyBuffer(CommandBuffer, Source, Destination, 1U, &BufferCopy);
    }
    FinishSingleCommandQueue(Queue, CommandPool, CommandBuffer);
}

void CreateVertexBuffer(Allocation::ObjectAllocation& Object, VkDeviceSize const& AllocationSize, std::vector<Vertex> const& Vertices)
{
    if (g_Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    auto const& [FamilyIndex, Queue] = GetTransferQueue();

    constexpr VkBufferUsageFlags SourceUsageFlags             = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    constexpr VkBufferUsageFlags DestinationUsageFlags             = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = 0U;

    VkBuffer StagingBuffer            = VK_NULL_HANDLE;
    VmaAllocation StagingBufferMemory = VK_NULL_HANDLE;
    VmaAllocationInfo const
            StagingInfo
            = CreateBuffer(
                    AllocationSize,
                    SourceUsageFlags,
                    SourceMemoryPropertyFlags,
                    StagingBuffer,
                    StagingBufferMemory);

    std::memcpy(StagingInfo.pMappedData, std::data(Vertices), AllocationSize);

    CreateBuffer(AllocationSize, DestinationUsageFlags, DestinationMemoryPropertyFlags, Object.VertexBuffer.Buffer, Object.VertexBuffer.Allocation);
    CopyBuffer(StagingBuffer, Object.VertexBuffer.Buffer, AllocationSize, Queue, FamilyIndex);

    vmaDestroyBuffer(g_Allocator, StagingBuffer, StagingBufferMemory);
}

void CreateIndexBuffer(Allocation::ObjectAllocation& Object, VkDeviceSize const& AllocationSize, std::vector<std::uint32_t> const& Indices)
{
    if (g_Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    auto const& [FamilyIndex, Queue] = GetTransferQueue();

    constexpr VkBufferUsageFlags SourceUsageFlags             = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    constexpr VkBufferUsageFlags DestinationUsageFlags             = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = 0U;

    VkBuffer StagingBuffer            = VK_NULL_HANDLE;
    VmaAllocation StagingBufferMemory = VK_NULL_HANDLE;
    VmaAllocationInfo const
            StagingInfo
            = CreateBuffer(
                    AllocationSize,
                    SourceUsageFlags,
                    SourceMemoryPropertyFlags,
                    StagingBuffer,
                    StagingBufferMemory);

    std::memcpy(StagingInfo.pMappedData, std::data(Indices), AllocationSize);
    CheckVulkanResult(vmaFlushAllocation(g_Allocator, StagingBufferMemory, 0U, AllocationSize));

    CreateBuffer(AllocationSize, DestinationUsageFlags, DestinationMemoryPropertyFlags, Object.IndexBuffer.Buffer, Object.IndexBuffer.Allocation);
    CopyBuffer(StagingBuffer, Object.IndexBuffer.Buffer, AllocationSize, Queue, FamilyIndex);

    vmaDestroyBuffer(g_Allocator, StagingBuffer, StagingBufferMemory);
}

void CreateUniformBuffer(Allocation::ObjectAllocation& Object, VkDeviceSize const& AllocationSize)
{
    if (g_Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    constexpr VkBufferUsageFlags DestinationUsageFlags             = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    CreateBuffer(AllocationSize, DestinationUsageFlags, DestinationMemoryPropertyFlags, Object.UniformBuffer.Buffer, Object.UniformBuffer.Allocation);
    vmaMapMemory(g_Allocator, Object.UniformBuffer.Allocation, &Object.UniformBuffer.MappedData);
}

void CreateImage(VkFormat const& ImageFormat,
                 VkExtent2D const& Extent,
                 VkImageTiling const& Tiling,
                 VkImageUsageFlags const Usage,
                 VkMemoryPropertyFlags const Flags,
                 VkImage& Image,
                 VmaAllocation& Allocation)
{
    if (g_Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    VkImageCreateInfo const ImageViewCreateInfo {
            .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format    = ImageFormat,
            .extent    = {
                       .width  = Extent.width,
                       .height = Extent.height,
                       .depth  = 1U},
            .mipLevels     = 1U,
            .arrayLayers   = 1U,
            .samples       = VK_SAMPLE_COUNT_1_BIT,
            .tiling        = Tiling,
            .usage         = Usage,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};

    VmaAllocationCreateInfo const ImageCreateInfo {
            .flags = Flags,
            .usage = VMA_MEMORY_USAGE_AUTO};

    VmaAllocationInfo AllocationInfo;
    CheckVulkanResult(vmaCreateImage(g_Allocator, &ImageViewCreateInfo, &ImageCreateInfo, &Image, &Allocation, &AllocationInfo));
}

void CreateImageView(VkImage const& Image, VkFormat const& Format, VkImageAspectFlags const& AspectFlags, VkImageView& ImageView)
{
    VkImageViewCreateInfo const ImageViewCreateInfo {
            .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image      = Image,
            .viewType   = VK_IMAGE_VIEW_TYPE_2D,
            .format     = Format,
            .components = {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY},
            .subresourceRange = {.aspectMask = AspectFlags, .baseMipLevel = 0U, .levelCount = 1U, .baseArrayLayer = 0U, .layerCount = 1U}};

    CheckVulkanResult(vkCreateImageView(volkGetLoadedDevice(), &ImageViewCreateInfo, nullptr, &ImageView));
}

void CreateSwapChainImageViews(VkFormat const& ImageFormat)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan swap chain image views";
    for (auto& [Image, View, Sampler, Allocation, Type]: g_SwapChainImages)
    {
        CreateImageView(Image, ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, View);
    }
}

void CreateTextureImageView(Allocation::ImageAllocation& Allocation)
{
    CreateImageView(Allocation.Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, Allocation.View);
}

void CreateTextureSampler(Allocation::ImageAllocation& Allocation)
{
    if (g_Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    VkPhysicalDeviceProperties DeviceProperties;
    vkGetPhysicalDeviceProperties(g_Allocator->GetPhysicalDevice(), &DeviceProperties);

    VkSamplerCreateInfo const SamplerCreateInfo {
            .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter               = VK_FILTER_LINEAR,
            .minFilter               = VK_FILTER_LINEAR,
            .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias              = 0.F,
            .anisotropyEnable        = VK_TRUE,
            .maxAnisotropy           = DeviceProperties.limits.maxSamplerAnisotropy,
            .compareEnable           = VK_FALSE,
            .compareOp               = VK_COMPARE_OP_ALWAYS,
            .minLod                  = 0.F,
            .maxLod                  = FLT_MAX,
            .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE};

    CheckVulkanResult(vkCreateSampler(volkGetLoadedDevice(), &SamplerCreateInfo, nullptr, &Allocation.Sampler));
}

void CopyBufferToImage(VkBuffer const& Source, VkImage const& Destination, VkExtent2D const& Extent, VkQueue const& Queue, std::uint32_t const QueueFamilyIndex)
{
    VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
    VkCommandPool CommandPool     = VK_NULL_HANDLE;
    InitializeSingleCommandQueue(CommandPool, CommandBuffer, static_cast<std::uint8_t>(QueueFamilyIndex));
    {
        VkBufferImageCopy const BufferImageCopy {
                .bufferOffset      = 0U,
                .bufferRowLength   = 0U,
                .bufferImageHeight = 0U,
                .imageSubresource  = {
                         .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                         .mipLevel       = 0U,
                         .baseArrayLayer = 0U,
                         .layerCount     = 1U},
                .imageOffset = {.x = 0U, .y = 0U, .z = 0U},
                .imageExtent = {.width = Extent.width, .height = Extent.height, .depth = 1U}};

        vkCmdCopyBufferToImage(CommandBuffer, Source, Destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &BufferImageCopy);
    }
    FinishSingleCommandQueue(Queue, CommandPool, CommandBuffer);
}

void MoveImageLayout(VkImage const& Image,
                     VkFormat const& Format,
                     VkImageLayout const& OldLayout,
                     VkImageLayout const& NewLayout,
                     VkQueue const& Queue,
                     std::uint8_t const QueueFamilyIndex)
{
    VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
    VkCommandPool CommandPool     = VK_NULL_HANDLE;
    InitializeSingleCommandQueue(CommandPool, CommandBuffer, QueueFamilyIndex);
    {
        VkPipelineStageFlags SourceStage;
        VkPipelineStageFlags DestinationStage;

        VkImageMemoryBarrier Barrier = {
                .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .oldLayout           = OldLayout,
                .newLayout           = NewLayout,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = Image,
                .subresourceRange    = {
                           .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                           .baseMipLevel   = 0U,
                           .levelCount     = 1U,
                           .baseArrayLayer = 0U,
                           .layerCount     = 1U}};

        if (NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            if (Format == VK_FORMAT_D32_SFLOAT_S8_UINT || Format == VK_FORMAT_D24_UNORM_S8_UINT)
            {
                Barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }

        if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            Barrier.srcAccessMask = 0;
            Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            SourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            DestinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            SourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
            DestinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            Barrier.srcAccessMask = 0;
            Barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            SourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            DestinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else
        {
            throw std::invalid_argument("Vulkan image layout transition is invalid");
        }

        vkCmdPipelineBarrier(CommandBuffer, SourceStage, DestinationStage, 0U, 0U, nullptr, 0U, nullptr, 1U, &Barrier);
    }
    FinishSingleCommandQueue(Queue, CommandPool, CommandBuffer);
}

Allocation::ImageAllocation AllocateTexture(unsigned char const* Data, std::uint32_t const Width, std::uint32_t const Height, std::size_t const AllocationSize)
{
    Allocation::ImageAllocation ImageAllocation {};

    constexpr VkBufferUsageFlags SourceUsageFlags             = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    constexpr VkImageUsageFlags DestinationUsageFlags              = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    constexpr VkImageTiling Tiling = VK_IMAGE_TILING_OPTIMAL;

    VkBuffer StagingBuffer            = nullptr;
    VmaAllocation StagingBufferMemory = nullptr;
    VmaAllocationInfo const
            StagingInfo
            = CreateBuffer(
                    std::clamp(AllocationSize, g_ImageBufferMemoryAllocationSize, UINT64_MAX),
                    SourceUsageFlags,
                    SourceMemoryPropertyFlags,
                    StagingBuffer,
                    StagingBufferMemory);

    std::memcpy(StagingInfo.pMappedData, Data, AllocationSize);

    constexpr VkFormat ImageFormat            = VK_FORMAT_R8G8B8A8_SRGB;
    constexpr VkImageLayout InitialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    constexpr VkImageLayout MiddleLayout      = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    constexpr VkImageLayout DestinationLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkExtent2D const Extent {
            .width  = Width,
            .height = Height};

    auto const& [FamilyIndex, Queue] = GetGraphicsQueue();

    CreateImage(ImageFormat, Extent, Tiling, DestinationUsageFlags, DestinationMemoryPropertyFlags, ImageAllocation.Image, ImageAllocation.Allocation);
    MoveImageLayout(ImageAllocation.Image, ImageFormat, InitialLayout, MiddleLayout, Queue, FamilyIndex);
    CopyBufferToImage(StagingBuffer, ImageAllocation.Image, Extent, Queue, FamilyIndex);
    MoveImageLayout(ImageAllocation.Image, ImageFormat, MiddleLayout, DestinationLayout, Queue, FamilyIndex);

    CreateTextureImageView(ImageAllocation);
    CreateTextureSampler(ImageAllocation);
    vmaDestroyBuffer(g_Allocator, StagingBuffer, StagingBufferMemory);

    return ImageAllocation;
}

void LoadTexture(Allocation::ObjectAllocation& Object, std::string_view const& TexturePath)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan texture image";

    if (g_Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    std::int32_t Width    = -1;
    std::int32_t Height   = -1;
    std::int32_t Channels = -1;

    if (!std::filesystem::exists(TexturePath))
    {
        throw std::runtime_error("Texture path is invalid. Path: " + std::string(TexturePath));
    }

    stbi_uc const* const ImagePixels = stbi_load(std::data(TexturePath), &Width, &Height, &Channels, STBI_rgb_alpha);
    auto const AllocationSize        = static_cast<VkDeviceSize>(Width) * static_cast<VkDeviceSize>(Height) * 4U;

    if (ImagePixels == nullptr)
    {
        throw std::runtime_error("STB ImageAllocation is invalid. Path: " + std::string(TexturePath));
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loaded image from path: '" << TexturePath << "'";

    Object.TextureImages.push_back(AllocateTexture(ImagePixels, static_cast<std::uint32_t>(Width), static_cast<std::uint32_t>(Height), AllocationSize));
}

void RenderCore::CreateVulkanSurface(GLFWwindow* const Window)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan surface";

    CheckVulkanResult(glfwCreateWindowSurface(volkGetLoadedInstance(), Window, nullptr, &g_Surface));
}

void RenderCore::CreateMemoryAllocator()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan memory allocator";

    VmaVulkanFunctions const VulkanFunctions {
            .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
            .vkGetDeviceProcAddr   = vkGetDeviceProcAddr};

    VmaAllocatorCreateInfo const AllocatorInfo {
            .flags                          = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT,
            .physicalDevice                 = GetPhysicalDevice(),
            .device                         = volkGetLoadedDevice(),
            .preferredLargeHeapBlockSize    = 0U /*Default: 256 MiB*/,
            .pAllocationCallbacks           = nullptr,
            .pDeviceMemoryCallbacks         = nullptr,
            .pHeapSizeLimit                 = nullptr,
            .pVulkanFunctions               = &VulkanFunctions,
            .instance                       = volkGetLoadedInstance(),
            .vulkanApiVersion               = VK_API_VERSION_1_0,
            .pTypeExternalMemoryHandleTypes = nullptr};

    CheckVulkanResult(vmaCreateAllocator(&AllocatorInfo, &g_Allocator));
}

void RenderCore::CreateSwapChain()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan swap chain";

    DeviceProperties const& Properties = GetDeviceProperties();

    std::vector<std::uint32_t> const QueueFamilyIndices = GetUniqueQueueFamilyIndicesU32();
    auto const QueueFamilyIndicesCount                  = static_cast<std::uint32_t>(std::size(QueueFamilyIndices));

    g_OldSwapChain    = g_SwapChain;
    g_SwapChainExtent = Properties.Extent;

    VkSwapchainCreateInfoKHR const SwapChainCreateInfo {
            .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface               = GetSurface(),
            .minImageCount         = GetMinImageCount(),
            .imageFormat           = Properties.Format.format,
            .imageColorSpace       = Properties.Format.colorSpace,
            .imageExtent           = g_SwapChainExtent,
            .imageArrayLayers      = 1U,
            .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode      = QueueFamilyIndicesCount > 1U ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = QueueFamilyIndicesCount,
            .pQueueFamilyIndices   = std::data(QueueFamilyIndices),
            .preTransform          = Properties.Capabilities.currentTransform,
            .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode           = Properties.Mode,
            .clipped               = VK_TRUE,
            .oldSwapchain          = g_OldSwapChain};

    VkDevice const& VulkanLogicalDevice = volkGetLoadedDevice();

    CheckVulkanResult(vkCreateSwapchainKHR(VulkanLogicalDevice, &SwapChainCreateInfo, nullptr, &g_SwapChain));

    if (g_OldSwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(VulkanLogicalDevice, g_OldSwapChain, nullptr);
        g_OldSwapChain = VK_NULL_HANDLE;
    }

    std::uint32_t Count = 0U;
    CheckVulkanResult(vkGetSwapchainImagesKHR(VulkanLogicalDevice, g_SwapChain, &Count, nullptr));

    std::vector<VkImage> SwapChainImages(Count, VK_NULL_HANDLE);
    CheckVulkanResult(vkGetSwapchainImagesKHR(VulkanLogicalDevice, g_SwapChain, &Count, std::data(SwapChainImages)));

    g_SwapChainImages.resize(Count, Allocation::ImageAllocation());
    for (std::uint32_t Iterator = 0U; Iterator < Count; ++Iterator)
    {
        g_SwapChainImages.at(Iterator).Image = SwapChainImages.at(Iterator);
    }

    CreateSwapChainImageViews(Properties.Format.format);
}

void RenderCore::CreateFrameBuffers()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan frame buffers";

    VkDevice const& VulkanLogicalDevice = volkGetLoadedDevice();

    g_FrameBuffers.resize(std::size(g_SwapChainImages), VK_NULL_HANDLE);
    for (std::uint32_t Iterator = 0U; Iterator < static_cast<std::uint32_t>(std::size(g_FrameBuffers)); ++Iterator)
    {
        std::array const Attachments {
                g_SwapChainImages.at(Iterator).View,
                g_DepthImage.View};

        VkFramebufferCreateInfo const FrameBufferCreateInfo {
                .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass      = GetRenderPass(),
                .attachmentCount = static_cast<std::uint32_t>(std::size(Attachments)),
                .pAttachments    = std::data(Attachments),
                .width           = g_SwapChainExtent.width,
                .height          = g_SwapChainExtent.height,
                .layers          = 1U};

        CheckVulkanResult(vkCreateFramebuffer(VulkanLogicalDevice, &FrameBufferCreateInfo, nullptr, &g_FrameBuffers.at(Iterator)));
    }
}

void RenderCore::CreateDepthResources()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan depth resources";

    DeviceProperties const& Properties = GetDeviceProperties();
    auto const& [FamilyIndex, Queue]   = GetGraphicsQueue();

    constexpr VkImageTiling Tiling                      = VK_IMAGE_TILING_OPTIMAL;
    constexpr VkImageUsageFlagBits Usage                = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    constexpr VkMemoryPropertyFlags MemoryPropertyFlags = 0U;
    constexpr VkImageAspectFlagBits Aspect              = VK_IMAGE_ASPECT_DEPTH_BIT;
    constexpr VkImageLayout InitialLayout               = VK_IMAGE_LAYOUT_UNDEFINED;
    constexpr VkImageLayout DestinationLayout           = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    CreateImage(Properties.DepthFormat, g_SwapChainExtent, Tiling, Usage, MemoryPropertyFlags, g_DepthImage.Image, g_DepthImage.Allocation);
    CreateImageView(g_DepthImage.Image, Properties.DepthFormat, Aspect, g_DepthImage.View);
    MoveImageLayout(g_DepthImage.Image, Properties.DepthFormat, InitialLayout, DestinationLayout, Queue, FamilyIndex);
}

void CreateVertexBuffers(Allocation::ObjectAllocation& Object, std::vector<Vertex> const& Vertices)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan vertex buffers";

    VkDeviceSize const BufferSize = std::size(Vertices) * sizeof(Vertex);
    CreateVertexBuffer(Object, BufferSize, Vertices);
}

void CreateIndexBuffers(Allocation::ObjectAllocation& Object, std::vector<std::uint32_t> const& Indices)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan index buffers";

    VkDeviceSize const BufferSize = std::size(Indices) * sizeof(std::uint32_t);
    CreateIndexBuffer(Object, BufferSize, Indices);
}

void CreateUniformBuffers(Allocation::ObjectAllocation& Object)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan uniform buffers";

    constexpr VkDeviceSize BufferSize = sizeof(UniformBufferObject);
    CreateUniformBuffer(Object, BufferSize);
}

struct PreProcessedImageData
{
    std::string Name {"None"};
    std::uint32_t Width {64U};
    std::uint32_t Height {64U};
    std::vector<unsigned char> Content {std::vector<unsigned char>(Width * Height, 0U)};
};

struct PreProcessedMeshData
{
    std::vector<Vertex> Vertices {};
    std::vector<std::uint32_t> Indices {};
};

struct ObjectPreProcessedData
{
    std::uint32_t ID {};
    std::string Name {};
    Transform Transform {};

    PreProcessedMeshData MeshData {};
    std::unordered_map<TextureType, PreProcessedImageData> Textures {
            {TextureType::BaseColor, PreProcessedImageData()}};
};

std::vector<Object> RenderCore::AllocateScene(std::string_view const& ModelPath)
{
    tinygltf::Model Model {};
    tinygltf::TinyGLTF ModelLoader {};
    std::string Error {};
    std::string Warning {};

    std::filesystem::path const ModelFilepath(ModelPath);

    bool const LoadResult = ModelFilepath.extension() == ".gltf" ? ModelLoader.LoadASCIIFromFile(&Model, &Error, &Warning, std::data(ModelPath)) : ModelLoader.LoadBinaryFromFile(&Model, &Error, &Warning, std::data(ModelPath));
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

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loaded model from path: '" << ModelPath << "'";

    std::vector<ObjectPreProcessedData> LoadedMeshes {};
    LoadedMeshes.reserve(std::size(Model.meshes));

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loading scenes: " << std::size(Model.scenes);

    for (tinygltf::Node const& Node: Model.nodes)
    {
        std::int32_t const MeshIndex = Node.mesh;
        if (MeshIndex < 0)
        {
            continue;
        }

        tinygltf::Mesh const& Mesh = Model.meshes.at(MeshIndex);
        std::vector<ObjectPreProcessedData> Primitives {};
        Primitives.reserve(std::size(Mesh.primitives));

        std::vector<std::jthread> PrimitiveProccessThreads {};
        PrimitiveProccessThreads.reserve(std::size(Mesh.primitives));

        for (tinygltf::Primitive const& Primitive: Mesh.primitives)
        {
            PrimitiveProccessThreads.emplace_back([Primitive, Model, Mesh, &Primitives] {
                std::uint32_t const ObjectID = g_ObjectIDCounter.fetch_add(1U);

                ObjectPreProcessedData NewPrimitive {
                        .ID   = ObjectID,
                        .Name = std::format("{}_{:03d}", Mesh.name, ObjectID)};

                auto const StartVertexNum  = static_cast<std::uint32_t>(std::size(NewPrimitive.MeshData.Vertices));
                std::uint32_t NewVertexNum = StartVertexNum;

                auto const TryResizeVertexContainer = [&NewPrimitive, &NewVertexNum, StartVertexNum](std::uint32_t const IncreaseSize) {
                    if (NewVertexNum == StartVertexNum)
                    {
                        NewVertexNum += IncreaseSize;
                        NewPrimitive.MeshData.Vertices.resize(NewVertexNum);
                    }
                };

                if (Primitive.attributes.contains("POSITION"))
                {
                    tinygltf::Accessor const& PositionAccessor     = Model.accessors.at(Primitive.attributes.at("POSITION"));
                    tinygltf::BufferView const& PositionBufferView = Model.bufferViews.at(PositionAccessor.bufferView);
                    tinygltf::Buffer const& PositionBuffer         = Model.buffers.at(PositionBufferView.buffer);
                    auto const* PositionData                       = reinterpret_cast<const float*>(std::data(PositionBuffer.data) + PositionBufferView.byteOffset + PositionAccessor.byteOffset);
                    TryResizeVertexContainer(PositionAccessor.count);

                    for (std::uint32_t Iterator = 0U; Iterator < static_cast<std::uint32_t>(PositionAccessor.count); ++Iterator)
                    {
                        NewPrimitive.MeshData.Vertices.at(StartVertexNum + Iterator).Position = glm::vec3(PositionData[Iterator * 3], PositionData[Iterator * 3 + 1], PositionData[Iterator * 3 + 2]);
                    }
                }

                if (Primitive.attributes.contains("NORMAL"))
                {
                    tinygltf::Accessor const& NormalAccessor     = Model.accessors.at(Primitive.attributes.at("NORMAL"));
                    tinygltf::BufferView const& NormalBufferView = Model.bufferViews.at(NormalAccessor.bufferView);
                    tinygltf::Buffer const& NormalBuffer         = Model.buffers.at(NormalBufferView.buffer);
                    auto const* NormalData                       = reinterpret_cast<const float*>(std::data(NormalBuffer.data) + NormalBufferView.byteOffset + NormalAccessor.byteOffset);
                    TryResizeVertexContainer(NormalAccessor.count);

                    for (std::uint32_t Iterator = 0U; Iterator < static_cast<std::uint32_t>(NormalAccessor.count); ++Iterator)
                    {
                        NewPrimitive.MeshData.Vertices.at(StartVertexNum + Iterator).Normal = glm::vec3(NormalData[Iterator * 3], NormalData[Iterator * 3 + 1], NormalData[Iterator * 3 + 2]);
                    }
                }

                if (Primitive.attributes.contains("TEXCOORD_0"))
                {
                    tinygltf::Accessor const& TexCoordAccessor     = Model.accessors.at(Primitive.attributes.at("TEXCOORD_0"));
                    tinygltf::BufferView const& TexCoordBufferView = Model.bufferViews.at(TexCoordAccessor.bufferView);
                    tinygltf::Buffer const& TexCoordBuffer         = Model.buffers.at(TexCoordBufferView.buffer);
                    auto const* TexCoordData                       = reinterpret_cast<const float*>(std::data(TexCoordBuffer.data) + TexCoordBufferView.byteOffset + TexCoordAccessor.byteOffset);
                    TryResizeVertexContainer(TexCoordAccessor.count);

                    for (std::uint32_t Iterator = 0U; Iterator < static_cast<std::uint32_t>(TexCoordAccessor.count); ++Iterator)
                    {
                        NewPrimitive.MeshData.Vertices.at(StartVertexNum + Iterator).TextureCoordinate = glm::vec2(TexCoordData[Iterator * 2], TexCoordData[Iterator * 2 + 1]);
                    }
                }

                if (Primitive.indices >= 0)
                {
                    tinygltf::Accessor const& IndexAccessor     = Model.accessors.at(Primitive.indices);
                    tinygltf::BufferView const& IndexBufferView = Model.bufferViews.at(IndexAccessor.bufferView);
                    tinygltf::Buffer const& IndexBuffer         = Model.buffers.at(IndexBufferView.buffer);

                    std::uint32_t const IndicesOffset = std::size(NewPrimitive.MeshData.Indices);
                    NewPrimitive.MeshData.Indices.reserve(std::size(NewPrimitive.MeshData.Indices) + IndexAccessor.count);

                    auto const InsertIndice = [&NewPrimitive, IndicesOffset, IndexAccessor](auto const* Indices) {
                        for (std::uint32_t Iterator = 0U; Iterator < static_cast<std::uint32_t>(IndexAccessor.count); ++Iterator)
                        {
                            NewPrimitive.MeshData.Indices.push_back(static_cast<std::uint32_t>(Indices[Iterator]) + IndicesOffset);
                        }
                    };

                    switch (IndexAccessor.componentType)
                    {
                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                            InsertIndice(reinterpret_cast<const uint32_t*>(std::data(IndexBuffer.data) + IndexBufferView.byteOffset + IndexAccessor.byteOffset));
                            break;
                        }
                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                            InsertIndice(reinterpret_cast<const uint16_t*>(std::data(IndexBuffer.data) + IndexBufferView.byteOffset + IndexAccessor.byteOffset));
                            break;
                        }
                        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                            InsertIndice(std::data(IndexBuffer.data) + IndexBufferView.byteOffset + IndexAccessor.byteOffset);
                            break;
                        }
                        default:
                            break;
                    }
                }

                if (Primitive.material >= 0)
                {
                    auto const PushTextureData = [&NewPrimitive, Model](std::int32_t const TextureIndex, TextureType const TexType) {
                        if (TextureIndex >= 0)
                        {
                            if (auto const& Texture = Model.textures.at(TextureIndex); Texture.source >= 0)
                            {
                                auto const& Image = Model.images.at(Texture.source);

                                NewPrimitive.Textures.at(TexType) = {.Name    = Image.uri,
                                                                     .Width   = static_cast<std::uint32_t>(Image.width),
                                                                     .Height  = static_cast<std::uint32_t>(Image.height),
                                                                     .Content = Image.image};
                            }
                        }
                    };

                    tinygltf::Material const& Material = Model.materials.at(Primitive.material);
                    PushTextureData(Material.pbrMetallicRoughness.baseColorTexture.index, TextureType::BaseColor);
                }

                Primitives.push_back(std::move(NewPrimitive));
            });
        }

        for (std::jthread& ThreadIter: PrimitiveProccessThreads)
        {
            ThreadIter.join();
        }

        for (ObjectPreProcessedData& PrimitiveIter: Primitives)
        {
            if (std::empty(PrimitiveIter.MeshData.Vertices) || std::empty(PrimitiveIter.MeshData.Indices))
            {
                continue;
            }

            if (!std::empty(Node.translation))
            {
                PrimitiveIter.Transform.Position = glm::make_vec3(std::data(Node.translation));
            }

            if (!std::empty(Node.scale))
            {
                PrimitiveIter.Transform.Scale = glm::make_vec3(std::data(Node.scale));

                PrimitiveIter.Transform.Scale.X = std::clamp(PrimitiveIter.Transform.Scale.X, 0.01F, 100.F);
                PrimitiveIter.Transform.Scale.Y = std::clamp(PrimitiveIter.Transform.Scale.Y, 0.01F, 100.F);
                PrimitiveIter.Transform.Scale.Z = std::clamp(PrimitiveIter.Transform.Scale.Z, 0.01F, 100.F);
            }

            if (!std::empty(Node.rotation))
            {
                PrimitiveIter.Transform.Rotation = glm::make_quat(std::data(Node.rotation));
            }

            LoadedMeshes.push_back(PrimitiveIter);
        }
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loaded models: " << std::size(LoadedMeshes);

    std::vector<Object> Output;
    Output.reserve(std::size(LoadedMeshes));
    for (auto& [ID, Name, Transform, ObjectData, Textures]: LoadedMeshes)
    {
        Allocation::ObjectAllocation NewObject {
                .IndicesCount = static_cast<std::uint32_t>(std::size(ObjectData.Indices))};

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Allocating model with ID: '" << ID << "'; Vertices: '" << std::size(ObjectData.Vertices) << "'; Indices: '" << std::size(ObjectData.Indices);

        CreateVertexBuffers(NewObject, ObjectData.Vertices);
        CreateIndexBuffers(NewObject, ObjectData.Indices);
        CreateUniformBuffers(NewObject);

        for (auto const& [Type, Data]: Textures)
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Allocating texture '" << Data.Name << "' with width: '" << Data.Width << "' and height: '" << Data.Height << "'";

            NewObject.TextureImages.push_back(
                    AllocateTexture(std::data(Data.Content),
                                    Data.Width,
                                    Data.Height,
                                    std::size(Data.Content)));

            NewObject.TextureImages.back().Type = Type;
        }

        g_Objects.emplace(ID, NewObject);

        Output.emplace_back(ID, ModelPath, Name);
        Output.back().SetTransform(Transform);
    }

    return Output;
}

void RenderCore::ReleaseScene(std::vector<std::uint32_t> const& ObjectIDs)
{
    for (std::uint32_t const ObjectIDIter: ObjectIDs)
    {
        if (!g_Objects.contains(ObjectIDIter))
        {
            return;
        }

        g_Objects.at(ObjectIDIter).DestroyResources();
        g_Objects.erase(ObjectIDIter);
    }

    if (std::empty(g_Objects))
    {
        g_ObjectIDCounter = 0U;
    }
}

void RenderCore::ReleaseBufferResources()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Releasing vulkan buffer resources";

    VkDevice const& VulkanLogicalDevice = volkGetLoadedDevice();

    if (g_SwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(VulkanLogicalDevice, g_SwapChain, nullptr);
        g_SwapChain = VK_NULL_HANDLE;
    }

    DestroyBufferResources(true);

    vkDestroySurfaceKHR(volkGetLoadedInstance(), g_Surface, nullptr);
    g_Surface = VK_NULL_HANDLE;

    vmaDestroyAllocator(g_Allocator);
    g_Allocator = VK_NULL_HANDLE;
}

void RenderCore::DestroyBufferResources(bool const ClearScene)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying vulkan buffer resources";

    VkDevice const& VulkanLogicalDevice = volkGetLoadedDevice();

    for (Allocation::ImageAllocation& ImageViewIter: g_SwapChainImages)
    {
        ImageViewIter.DestroyResources();
    }
    g_SwapChainImages.clear();

    for (VkFramebuffer& FrameBufferIter: g_FrameBuffers)
    {
        if (FrameBufferIter != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(VulkanLogicalDevice, FrameBufferIter, nullptr);
            FrameBufferIter = VK_NULL_HANDLE;
        }
    }
    g_FrameBuffers.clear();

    g_DepthImage.DestroyResources();

    if (ClearScene)
    {
        for (auto& ObjectIter: g_Objects | std::views::values)
        {
            ObjectIter.DestroyResources();
        }
        g_Objects.clear();
    }
}

VkSurfaceKHR& RenderCore::GetSurface()
{
    return g_Surface;
}

VmaAllocator const& RenderCore::GetAllocator()
{
    return g_Allocator;
}

VkSwapchainKHR const& RenderCore::GetSwapChain()
{
    return g_SwapChain;
}

VkExtent2D const& RenderCore::GetSwapChainExtent()
{
    return g_SwapChainExtent;
}

std::vector<VkFramebuffer> const& RenderCore::GetFrameBuffers()
{
    return g_FrameBuffers;
}

VkBuffer RenderCore::GetVertexBuffer(std::uint32_t const ObjectID)
{
    if (!g_Objects.contains(ObjectID))
    {
        return VK_NULL_HANDLE;
    }

    return g_Objects.at(ObjectID).VertexBuffer.Buffer;
}

VkBuffer RenderCore::GetIndexBuffer(std::uint32_t const ObjectID)
{
    if (!g_Objects.contains(ObjectID))
    {
        return VK_NULL_HANDLE;
    }

    return g_Objects.at(ObjectID).IndexBuffer.Buffer;
}

std::uint32_t RenderCore::GetIndicesCount(std::uint32_t const ObjectID)
{
    if (!g_Objects.contains(ObjectID))
    {
        return 0U;
    }

    return g_Objects.at(ObjectID).IndicesCount;
}

void* RenderCore::GetUniformData(std::uint32_t const ObjectID)
{
    if (!g_Objects.contains(ObjectID))
    {
        return nullptr;
    }

    return g_Objects.at(ObjectID).UniformBuffer.MappedData;
}

bool RenderCore::ContainsObject(std::uint32_t const ID)
{
    return g_Objects.contains(ID);
}

std::vector<MeshBufferData> RenderCore::GetAllocatedObjects()
{
    std::vector<MeshBufferData> Output;
    for (auto const& [ID, Data]: g_Objects)
    {
        if (!Data.UniformBuffer.Allocation)
        {
            continue;
        }

        Output.push_back({.ID                = ID,
                          .UniformBuffer     = Data.UniformBuffer.Buffer,
                          .UniformBufferData = Data.UniformBuffer.Allocation->GetMappedData()});

        for (auto const& ImageAllocation: Data.TextureImages)
        {
            Output.back().Textures.emplace(ImageAllocation.Type,
                                           TextureBufferData {
                                                   .ImageView = ImageAllocation.View,
                                                   .Sampler   = ImageAllocation.Sampler});
        }
    }

    return Output;
}

std::uint32_t RenderCore::GetNumAllocations()
{
    return static_cast<std::uint32_t>(std::size(g_Objects));
}

std::uint32_t RenderCore::GetClampedNumAllocations()
{
    return std::clamp(static_cast<std::uint32_t>(std::size(g_Objects)), 1U, UINT32_MAX);
}

void RenderCore::UpdateUniformBuffers(std::uint32_t const ObjectID)
{
    auto const& [Width, Height] = GetSwapChainExtent();
    glm::mat4 Projection        = glm::perspective(glm::radians(45.F), static_cast<float>(Width) / static_cast<float>(Height), 0.1F, 100.F);
    Projection[1][1] *= -1;

    if (!ContainsObject(ObjectID))
    {
        return;
    }

    std::shared_ptr<Object> const Object = EngineCore::Get().GetObjectByID(ObjectID);
    if (!Object)
    {
        return;
    }

    if (void* UniformBufferData = GetUniformData(ObjectID); UniformBufferData != nullptr)
    {
        UniformBufferObject const UpdatedUBO {
                .Model      = Object->GetMatrix(),
                .View       = GetViewportCamera().GetMatrix(),
                .Projection = Projection};

        std::memcpy(UniformBufferData, &UpdatedUBO, sizeof(UniformBufferObject));
    }
}