// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <assimp/Importer.hpp>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <boost/log/trivial.hpp>
#include <volk.h>

#ifndef VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#endif
#include <vk_mem_alloc.h>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include <stb_image.h>

module RenderCore.Management.BufferManagement;

import <span>;
import <vector>;
import <ranges>;
import <cstdint>;
import <filesystem>;

import RenderCore.EngineCore;
import RenderCore.Management.DeviceManagement;
import RenderCore.Management.PipelineManagement;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Types.DeviceProperties;
import RenderCore.Types.Vertex;
import RenderCore.Types.TextureData;
import RenderCore.Types.ObjectData;
import RenderCore.Utils.Constants;
import RenderCore.Utils.Helpers;

using namespace RenderCore;

namespace Allocation
{
    struct ImageAllocation
    {
        VkImage Image {VK_NULL_HANDLE};
        VkImageView View {VK_NULL_HANDLE};
        VkSampler Sampler {VK_NULL_HANDLE};
        VmaAllocation Allocation {VK_NULL_HANDLE};

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
        ImageAllocation TextureImage {};
        BufferAllocation VertexBuffer {};
        BufferAllocation IndexBuffer {};
        BufferAllocation UniformBuffer {};
        std::uint32_t IndicesCount {0U};

        [[nodiscard]] bool IsValid() const
        {
            return TextureImage.IsValid() && VertexBuffer.IsValid() && IndexBuffer.IsValid() && IndicesCount != 0U;
        }

        void DestroyResources()
        {
            VertexBuffer.DestroyResources();
            IndexBuffer.DestroyResources();
            TextureImage.DestroyResources();
            UniformBuffer.DestroyResources();
            IndicesCount = 0U;
        }
    };
}// namespace Allocation

VmaAllocator g_Allocator {VK_NULL_HANDLE};
VkSwapchainKHR g_SwapChain {VK_NULL_HANDLE};
VkSwapchainKHR g_OldSwapChain {VK_NULL_HANDLE};
VkExtent2D g_SwapChainExtent {0U, 0U};
std::vector<Allocation::ImageAllocation> g_SwapChainImages {};
Allocation::ImageAllocation g_DepthImage {};
std::vector<VkFramebuffer> g_FrameBuffers {};
std::unordered_map<std::uint32_t, Allocation::ObjectAllocation> g_Objects {};
std::atomic<std::uint32_t> g_ObjectIDCounter {0U};

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

    std::memcpy(StagingInfo.pMappedData, Vertices.data(), AllocationSize);

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

    std::memcpy(StagingInfo.pMappedData, Indices.data(), AllocationSize);
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
    for (auto& [Image, View, Sampler, Allocation]: g_SwapChainImages)
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

void LoadTexture(Allocation::ObjectAllocation& Object, std::string_view const TexturePath)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan texture image";

    if (g_Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    std::int32_t Width    = -1;
    std::int32_t Height   = -1;
    std::int32_t Channels = -1;

    std::string UsedTexturePath = TexturePath.data();
    if (!std::filesystem::exists(UsedTexturePath))
    {
        UsedTexturePath = EMPTY_TEX;
    }

    stbi_uc const* const ImagePixels = stbi_load(UsedTexturePath.c_str(), &Width, &Height, &Channels, STBI_rgb_alpha);
    auto const AllocationSize        = static_cast<VkDeviceSize>(Width) * static_cast<VkDeviceSize>(Height) * 4U;

    if (ImagePixels == nullptr)
    {
        throw std::runtime_error("STB ImageAllocation is invalid. Path: " + std::string(TexturePath));
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loaded image from path: '" << TexturePath << "'";

    Object.TextureImage = AllocateTexture(ImagePixels, static_cast<std::uint32_t>(Width), static_cast<std::uint32_t>(Height), AllocationSize);
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
    auto const QueueFamilyIndicesCount                  = static_cast<std::uint32_t>(QueueFamilyIndices.size());

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
            .pQueueFamilyIndices   = QueueFamilyIndices.data(),
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
    CheckVulkanResult(vkGetSwapchainImagesKHR(VulkanLogicalDevice, g_SwapChain, &Count, SwapChainImages.data()));

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

    g_FrameBuffers.resize(g_SwapChainImages.size(), VK_NULL_HANDLE);
    for (std::uint32_t Iterator = 0U; Iterator < static_cast<std::uint32_t>(g_FrameBuffers.size()); ++Iterator)
    {
        std::array const Attachments {
                g_SwapChainImages.at(Iterator).View,
                g_DepthImage.View};

        VkFramebufferCreateInfo const FrameBufferCreateInfo {
                .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass      = GetRenderPass(),
                .attachmentCount = static_cast<std::uint32_t>(Attachments.size()),
                .pAttachments    = Attachments.data(),
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

    VkDeviceSize const BufferSize = Vertices.size() * sizeof(Vertex);
    CreateVertexBuffer(Object, BufferSize, Vertices);
}

void CreateIndexBuffers(Allocation::ObjectAllocation& Object, std::vector<std::uint32_t> const& Indices)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan index buffers";

    VkDeviceSize const BufferSize = Indices.size() * sizeof(std::uint32_t);
    CreateIndexBuffer(Object, BufferSize, Indices);
}

void CreateUniformBuffers(Allocation::ObjectAllocation& Object)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan uniform buffers";

    VkDeviceSize const BufferSize = sizeof(UniformBufferObject);
    CreateUniformBuffer(Object, BufferSize);
}

std::uint32_t RenderCore::LoadObject(std::string_view const ModelPath, std::string_view const TexturePath)
{
    Assimp::Importer Importer;
    aiScene const* const Scene = Importer.ReadFile(ModelPath.data(), (aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_FlipUVs));

    if (Scene == nullptr || (Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0U || Scene->mRootNode == nullptr)
    {
        throw std::runtime_error("Assimp error: " + std::string(Importer.GetErrorString()));
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loaded model from path: '" << ModelPath << "'";

    if (g_Objects.empty())
    {
        g_ObjectIDCounter = 0U;
    }

    std::uint32_t const NewID = g_ObjectIDCounter.fetch_add(1U);

    std::vector<Vertex> Vertices;
    std::vector<std::uint32_t> Indices;

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loading Meshes: '" << Scene->mNumMeshes << "'";

    std::span const AiMeshesSpan(Scene->mMeshes, Scene->mNumMeshes);
    for (std::vector const Meshes(AiMeshesSpan.begin(), AiMeshesSpan.end());
         aiMesh const* MeshIter: Meshes)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loading Vertices: '" << MeshIter->mNumVertices << "'";

        std::span const AiVerticesSpan(MeshIter->mVertices, MeshIter->mNumVertices);
        std::vector const VerticesContainer(AiVerticesSpan.begin(), AiVerticesSpan.end());

        for (auto VerticesIter = VerticesContainer.begin(); VerticesIter != VerticesContainer.end(); ++VerticesIter)
        {
            aiVector3D const& Position   = *VerticesIter;
            aiVector3D TextureCoordinate = {};
            aiVector3D Normal            = {};
            aiColor4D Color              = {1.F, 1.F, 1.F, 1.F};

            if (MeshIter->HasTextureCoords(0))
            {
                TextureCoordinate = MeshIter->mTextureCoords[0][VerticesIter - VerticesContainer.begin()];
            }

            if (MeshIter->HasNormals())
            {
                Normal = MeshIter->mNormals[VerticesIter - VerticesContainer.begin()];
            }

            if (MeshIter->HasVertexColors(0))
            {
                Color = MeshIter->mColors[0][VerticesIter - VerticesContainer.begin()];
            }

            Vertices.push_back(Vertex {
                    .Position          = {Position.x, Position.y, Position.z},
                    .Normal            = {Normal.x, Normal.y, Normal.z},
                    .TextureCoordinate = {TextureCoordinate.x, TextureCoordinate.y, TextureCoordinate.z},
                    .Color             = {Color.r, Color.g, Color.b, Color.a}});
        }

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loading Faces: '" << MeshIter->mNumFaces << "'";

        std::span const AiFacesSpan(MeshIter->mFaces, MeshIter->mNumFaces);
        for (std::vector const Faces(AiFacesSpan.begin(), AiFacesSpan.end());
             aiFace const& Face: Faces)
        {
            if (Face.mNumIndices != 3U)
            {
                continue;
            }

            std::span const AiIndicesSpan(Face.mIndices, Face.mNumIndices);
            for (std::vector const AiIndices(AiIndicesSpan.begin(), AiIndicesSpan.end());
                 std::uint32_t const IndiceIter: AiIndices)
            {
                Indices.push_back(IndiceIter);
            }
        }
    }

    Allocation::ObjectAllocation NewObject {
            .IndicesCount = static_cast<std::uint32_t>(Indices.size())};

    CreateVertexBuffers(NewObject, Vertices);
    CreateIndexBuffers(NewObject, Indices);
    CreateUniformBuffers(NewObject);
    LoadTexture(NewObject, TexturePath);

    g_Objects.emplace(NewID, NewObject);

    return NewID;
}

void RenderCore::UnLoadObject(std::uint32_t const ObjectID)
{
    if (!g_Objects.contains(ObjectID))
    {
        return;
    }

    g_Objects.at(ObjectID).DestroyResources();
    g_Objects.erase(ObjectID);
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

std::vector<TextureData> RenderCore::GetAllocatedTextures()
{
    std::vector<TextureData> Output;
    for (auto const& [Key, Value]: g_Objects)
    {
        Output.push_back(
                TextureData {
                        .ObjectID  = Key,
                        .ImageView = Value.TextureImage.View,
                        .Sampler   = Value.TextureImage.Sampler});
    }

    return Output;
}

std::vector<ObjectData> RenderCore::GetAllocatedObjects()
{
    std::vector<ObjectData> Output;
    for (auto const& [Key, Value]: g_Objects)
    {
        if (!Value.UniformBuffer.Allocation)
        {
            continue;
        }

        Output.push_back(
                ObjectData {
                        .ObjectID          = Key,
                        .UniformBuffer     = Value.UniformBuffer.Buffer,
                        .UniformBufferData = Value.UniformBuffer.Allocation->GetMappedData()});
    }

    return Output;
}