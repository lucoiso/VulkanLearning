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

import RenderCore.Renderer;
import RenderCore.Management.CommandsManagement;
import RenderCore.Management.DeviceManagement;
import RenderCore.Management.PipelineManagement;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Types.Vertex;
import RenderCore.Types.Camera;
import RenderCore.Types.Object;
import Timer.ExecutionCounter;

using namespace RenderCore;

VmaAllocationInfo CreateBuffer(VmaAllocator const& Allocator, VkDeviceSize const& Size, VkBufferUsageFlags const Usage, VkMemoryPropertyFlags const Flags, VkBuffer& Buffer, VmaAllocation& Allocation)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    if (Allocator == VK_NULL_HANDLE)
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
    CheckVulkanResult(vmaCreateBuffer(Allocator, &BufferCreateInfo, &AllocationCreateInfo, &Buffer, &Allocation, &MemoryAllocationInfo));

    return MemoryAllocationInfo;
}

void CopyBuffer(VkDevice const& LogicalDevice, VkBuffer const& Source, VkBuffer const& Destination, VkDeviceSize const& Size, VkQueue const& Queue, std::uint8_t const QueueFamilyIndex)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
    VkCommandPool CommandPool     = VK_NULL_HANDLE;
    InitializeSingleCommandQueue(LogicalDevice, CommandPool, CommandBuffer, QueueFamilyIndex);
    {
        VkBufferCopy const BufferCopy {
                .size = Size,
        };

        vkCmdCopyBuffer(CommandBuffer, Source, Destination, 1U, &BufferCopy);
    }
    FinishSingleCommandQueue(LogicalDevice, Queue, CommandPool, CommandBuffer);
}

void CreateVertexBuffer(VkDevice const& LogicalDevice, VmaAllocator const& Allocator, std::pair<std::uint8_t, VkQueue> const& QueuePair, ObjectAllocation& Object, VkDeviceSize const& AllocationSize, std::vector<Vertex> const& Vertices)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    if (Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    auto const& [FamilyIndex, Queue] = QueuePair;

    constexpr VkBufferUsageFlags SourceUsageFlags             = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    constexpr VkBufferUsageFlags DestinationUsageFlags             = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = 0U;

    VkBuffer StagingBuffer            = VK_NULL_HANDLE;
    VmaAllocation StagingBufferMemory = VK_NULL_HANDLE;
    VmaAllocationInfo const
            StagingInfo
            = CreateBuffer(Allocator,
                           AllocationSize,
                           SourceUsageFlags,
                           SourceMemoryPropertyFlags,
                           StagingBuffer,
                           StagingBufferMemory);

    std::memcpy(StagingInfo.pMappedData, std::data(Vertices), AllocationSize);

    CreateBuffer(Allocator, AllocationSize, DestinationUsageFlags, DestinationMemoryPropertyFlags, Object.VertexBuffer.Buffer, Object.VertexBuffer.Allocation);
    CopyBuffer(LogicalDevice, StagingBuffer, Object.VertexBuffer.Buffer, AllocationSize, Queue, FamilyIndex);

    vmaDestroyBuffer(Allocator, StagingBuffer, StagingBufferMemory);
}

void CreateIndexBuffer(VkDevice const& LogicalDevice, VmaAllocator const& Allocator, std::pair<std::uint8_t, VkQueue> const& QueuePair, ObjectAllocation& Object, VkDeviceSize const& AllocationSize, std::vector<std::uint32_t> const& Indices)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    if (Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    auto const& [FamilyIndex, Queue] = QueuePair;

    constexpr VkBufferUsageFlags SourceUsageFlags             = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    constexpr VkBufferUsageFlags DestinationUsageFlags             = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = 0U;

    VkBuffer StagingBuffer            = VK_NULL_HANDLE;
    VmaAllocation StagingBufferMemory = VK_NULL_HANDLE;
    VmaAllocationInfo const
            StagingInfo
            = CreateBuffer(Allocator,
                           AllocationSize,
                           SourceUsageFlags,
                           SourceMemoryPropertyFlags,
                           StagingBuffer,
                           StagingBufferMemory);

    std::memcpy(StagingInfo.pMappedData, std::data(Indices), AllocationSize);
    CheckVulkanResult(vmaFlushAllocation(Allocator, StagingBufferMemory, 0U, AllocationSize));

    CreateBuffer(Allocator, AllocationSize, DestinationUsageFlags, DestinationMemoryPropertyFlags, Object.IndexBuffer.Buffer, Object.IndexBuffer.Allocation);
    CopyBuffer(LogicalDevice, StagingBuffer, Object.IndexBuffer.Buffer, AllocationSize, Queue, FamilyIndex);

    vmaDestroyBuffer(Allocator, StagingBuffer, StagingBufferMemory);
}

void CreateUniformBuffer(VmaAllocator const& Allocator, ObjectAllocation& Object, VkDeviceSize const& AllocationSize)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    if (Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    constexpr VkBufferUsageFlags DestinationUsageFlags             = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    CreateBuffer(Allocator, AllocationSize, DestinationUsageFlags, DestinationMemoryPropertyFlags, Object.UniformBuffer.Buffer, Object.UniformBuffer.Allocation);
    vmaMapMemory(Allocator, Object.UniformBuffer.Allocation, &Object.UniformBuffer.MappedData);
}

void CreateVertexBuffers(VkDevice const& LogicalDevice, VmaAllocator const& Allocator, std::pair<std::uint8_t, VkQueue> const& QueuePair, ObjectAllocation& Object, std::vector<Vertex> const& Vertices)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan vertex buffers";

    VkDeviceSize const BufferSize = std::size(Vertices) * sizeof(Vertex);
    CreateVertexBuffer(LogicalDevice, Allocator, QueuePair, Object, BufferSize, Vertices);
}

void CreateIndexBuffers(VkDevice const& LogicalDevice, VmaAllocator const& Allocator, std::pair<std::uint8_t, VkQueue> const& QueuePair, ObjectAllocation& Object, std::vector<std::uint32_t> const& Indices)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan index buffers";

    VkDeviceSize const BufferSize = std::size(Indices) * sizeof(std::uint32_t);
    CreateIndexBuffer(LogicalDevice, Allocator, QueuePair, Object, BufferSize, Indices);
}

void CreateUniformBuffers(VmaAllocator const& Allocator, ObjectAllocation& Object)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan uniform buffers";

    constexpr VkDeviceSize BufferSize = sizeof(UniformBufferObject);
    CreateUniformBuffer(Allocator, Object, BufferSize);
}

void CreateImage(VmaAllocator const& Allocator,
                 VkFormat const& ImageFormat,
                 VkExtent2D const& Extent,
                 VkImageTiling const& Tiling,
                 VkImageUsageFlags const Usage,
                 VkMemoryPropertyFlags const Flags,
                 VkImage& Image,
                 VmaAllocation& Allocation)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    if (Allocator == VK_NULL_HANDLE)
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
    CheckVulkanResult(vmaCreateImage(Allocator, &ImageViewCreateInfo, &ImageCreateInfo, &Image, &Allocation, &AllocationInfo));
}

void CreateImageView(VkDevice const& LogicalDevice, VkImage const& Image, VkFormat const& Format, VkImageAspectFlags const& AspectFlags, VkImageView& ImageView)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

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

    CheckVulkanResult(vkCreateImageView(LogicalDevice, &ImageViewCreateInfo, nullptr, &ImageView));
}

void CreateSwapChainImageViews(VkDevice const& LogicalDevice, std::vector<ImageAllocation>& Images, VkFormat const& ImageFormat)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan swap chain image views";
    for (auto& [Image, View, Sampler, Allocation, Type]: Images)
    {
        CreateImageView(LogicalDevice, Image, ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, View);
    }
}

void CreateTextureImageView(VkDevice const& LogicalDevice, ImageAllocation& Allocation)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    CreateImageView(LogicalDevice, Allocation.Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, Allocation.View);
}

void CreateTextureSampler(VkDevice const& LogicalDevice, VmaAllocator const& Allocator, ImageAllocation& Allocation)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    if (Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    VkPhysicalDeviceProperties DeviceProperties;
    vkGetPhysicalDeviceProperties(Allocator->GetPhysicalDevice(), &DeviceProperties);

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

    CheckVulkanResult(vkCreateSampler(LogicalDevice, &SamplerCreateInfo, nullptr, &Allocation.Sampler));
}

void CopyBufferToImage(VkDevice const& LogicalDevice, VkBuffer const& Source, VkImage const& Destination, VkExtent2D const& Extent, VkQueue const& Queue, std::uint32_t const QueueFamilyIndex)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
    VkCommandPool CommandPool     = VK_NULL_HANDLE;
    InitializeSingleCommandQueue(LogicalDevice, CommandPool, CommandBuffer, static_cast<std::uint8_t>(QueueFamilyIndex));
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
    FinishSingleCommandQueue(LogicalDevice, Queue, CommandPool, CommandBuffer);
}

template<VkImageLayout OldLayout, VkImageLayout NewLayout>
constexpr void MoveImageLayout(VkDevice const& LogicalDevice, VkImage const& Image,
                               VkFormat const& Format,
                               VkQueue const& Queue,
                               std::uint8_t const QueueFamilyIndex)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
    VkCommandPool CommandPool     = VK_NULL_HANDLE;
    InitializeSingleCommandQueue(LogicalDevice, CommandPool, CommandBuffer, QueueFamilyIndex);
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

        if constexpr (NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            if (Format == VK_FORMAT_D32_SFLOAT_S8_UINT || Format == VK_FORMAT_D24_UNORM_S8_UINT)
            {
                Barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }

        if constexpr (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            Barrier.srcAccessMask = 0;
            Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            SourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            DestinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if constexpr (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            SourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
            DestinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if constexpr (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
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
    FinishSingleCommandQueue(LogicalDevice, Queue, CommandPool, CommandBuffer);
}

ImageAllocation AllocateTexture(VkDevice const& LogicalDevice, VmaAllocator const& Allocator, std::pair<std::uint8_t, VkQueue> const& QueuePair, unsigned char const* Data, std::uint32_t const Width, std::uint32_t const Height, std::size_t const AllocationSize)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    ImageAllocation ImageAllocation {};

    constexpr VkBufferUsageFlags SourceUsageFlags             = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    constexpr VkImageUsageFlags DestinationUsageFlags              = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    constexpr VkImageTiling Tiling = VK_IMAGE_TILING_OPTIMAL;

    VkBuffer StagingBuffer            = nullptr;
    VmaAllocation StagingBufferMemory = nullptr;
    VmaAllocationInfo const
            StagingInfo
            = CreateBuffer(Allocator,
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

    auto const& [FamilyIndex, Queue] = QueuePair;

    CreateImage(Allocator, ImageFormat, Extent, Tiling, DestinationUsageFlags, DestinationMemoryPropertyFlags, ImageAllocation.Image, ImageAllocation.Allocation);
    MoveImageLayout<InitialLayout, MiddleLayout>(LogicalDevice, ImageAllocation.Image, ImageFormat, Queue, FamilyIndex);
    CopyBufferToImage(LogicalDevice, StagingBuffer, ImageAllocation.Image, Extent, Queue, FamilyIndex);
    MoveImageLayout<MiddleLayout, DestinationLayout>(LogicalDevice, ImageAllocation.Image, ImageFormat, Queue, FamilyIndex);

    CreateTextureImageView(LogicalDevice, ImageAllocation);
    CreateTextureSampler(LogicalDevice, Allocator, ImageAllocation);
    vmaDestroyBuffer(Allocator, StagingBuffer, StagingBufferMemory);

    return ImageAllocation;
}

void LoadTexture(VkDevice const& LogicalDevice, VmaAllocator const& Allocator, std::pair<std::uint8_t, VkQueue> const& QueuePair, ObjectAllocation& Object, std::string_view const& TexturePath)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan texture image";

    if (Allocator == VK_NULL_HANDLE)
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

    Object.TextureImages.push_back(AllocateTexture(LogicalDevice, Allocator, QueuePair, ImagePixels, static_cast<std::uint32_t>(Width), static_cast<std::uint32_t>(Height), AllocationSize));
}

void BufferManager::CreateVulkanSurface(GLFWwindow* const Window)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan surface";

    CheckVulkanResult(glfwCreateWindowSurface(volkGetLoadedInstance(), Window, nullptr, &m_Surface));
}

void BufferManager::CreateMemoryAllocator(VkDevice const& LogicalDevice, VkPhysicalDevice const& PhysicalDevice)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan memory allocator";

    VmaVulkanFunctions const VulkanFunctions {
            .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
            .vkGetDeviceProcAddr   = vkGetDeviceProcAddr};

    VmaAllocatorCreateInfo const AllocatorInfo {
            .flags                          = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT,
            .physicalDevice                 = PhysicalDevice,
            .device                         = LogicalDevice,
            .preferredLargeHeapBlockSize    = 0U /*Default: 256 MiB*/,
            .pAllocationCallbacks           = nullptr,
            .pDeviceMemoryCallbacks         = nullptr,
            .pHeapSizeLimit                 = nullptr,
            .pVulkanFunctions               = &VulkanFunctions,
            .instance                       = volkGetLoadedInstance(),
            .vulkanApiVersion               = VK_API_VERSION_1_0,
            .pTypeExternalMemoryHandleTypes = nullptr};

    CheckVulkanResult(vmaCreateAllocator(&AllocatorInfo, &m_Allocator));
}

void BufferManager::CreateSwapChain(VkDevice const& LogicalDevice, DeviceProperties const& DeviceProperties, std::vector<std::uint32_t> const& QueueFamilyIndices, std::uint32_t const MinImageCount)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan swap chain";

    auto const QueueFamilyIndicesCount = static_cast<std::uint32_t>(std::size(QueueFamilyIndices));

    m_OldSwapChain    = m_SwapChain;
    m_SwapChainExtent = DeviceProperties.Extent;

    VkSwapchainCreateInfoKHR const SwapChainCreateInfo {
            .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface               = GetSurface(),
            .minImageCount         = MinImageCount,
            .imageFormat           = DeviceProperties.Format.format,
            .imageColorSpace       = DeviceProperties.Format.colorSpace,
            .imageExtent           = m_SwapChainExtent,
            .imageArrayLayers      = 1U,
            .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode      = QueueFamilyIndicesCount > 1U ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = QueueFamilyIndicesCount,
            .pQueueFamilyIndices   = std::data(QueueFamilyIndices),
            .preTransform          = DeviceProperties.Capabilities.currentTransform,
            .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode           = DeviceProperties.Mode,
            .clipped               = VK_TRUE,
            .oldSwapchain          = m_OldSwapChain};


    CheckVulkanResult(vkCreateSwapchainKHR(LogicalDevice, &SwapChainCreateInfo, nullptr, &m_SwapChain));

    if (m_OldSwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(LogicalDevice, m_OldSwapChain, nullptr);
        m_OldSwapChain = VK_NULL_HANDLE;
    }

    std::uint32_t Count = 0U;
    CheckVulkanResult(vkGetSwapchainImagesKHR(LogicalDevice, m_SwapChain, &Count, nullptr));

    std::vector<VkImage> SwapChainImages(Count, VK_NULL_HANDLE);
    CheckVulkanResult(vkGetSwapchainImagesKHR(LogicalDevice, m_SwapChain, &Count, std::data(SwapChainImages)));

    m_SwapChainImages.resize(Count, ImageAllocation());
    for (std::uint32_t Iterator = 0U; Iterator < Count; ++Iterator)
    {
        m_SwapChainImages.at(Iterator).Image = SwapChainImages.at(Iterator);
    }

    CreateSwapChainImageViews(LogicalDevice, m_SwapChainImages, DeviceProperties.Format.format);
}

void BufferManager::CreateFrameBuffers(VkDevice const& LogicalDevice, VkRenderPass const& RenderPass)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan frame buffers";


    m_FrameBuffers.resize(std::size(m_SwapChainImages), VK_NULL_HANDLE);
    for (std::uint32_t Iterator = 0U; Iterator < static_cast<std::uint32_t>(std::size(m_FrameBuffers)); ++Iterator)
    {
        std::array const Attachments {
                m_SwapChainImages.at(Iterator).View,
                m_DepthImage.View};

        VkFramebufferCreateInfo const FrameBufferCreateInfo {
                .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass      = RenderPass,
                .attachmentCount = static_cast<std::uint32_t>(std::size(Attachments)),
                .pAttachments    = std::data(Attachments),
                .width           = m_SwapChainExtent.width,
                .height          = m_SwapChainExtent.height,
                .layers          = 1U};

        CheckVulkanResult(vkCreateFramebuffer(LogicalDevice, &FrameBufferCreateInfo, nullptr, &m_FrameBuffers.at(Iterator)));
    }
}

void BufferManager::CreateDepthResources(VkDevice const& LogicalDevice, DeviceProperties const& DeviceProperties, std::pair<std::uint8_t, VkQueue> const& QueuePair)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan depth resources";

    auto const& [FamilyIndex, Queue] = QueuePair;

    constexpr VkImageTiling Tiling                      = VK_IMAGE_TILING_OPTIMAL;
    constexpr VkImageUsageFlagBits Usage                = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    constexpr VkMemoryPropertyFlags MemoryPropertyFlags = 0U;
    constexpr VkImageAspectFlagBits Aspect              = VK_IMAGE_ASPECT_DEPTH_BIT;
    constexpr VkImageLayout InitialLayout               = VK_IMAGE_LAYOUT_UNDEFINED;
    constexpr VkImageLayout DestinationLayout           = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    CreateImage(m_Allocator, DeviceProperties.DepthFormat, m_SwapChainExtent, Tiling, Usage, MemoryPropertyFlags, m_DepthImage.Image, m_DepthImage.Allocation);
    CreateImageView(LogicalDevice, m_DepthImage.Image, DeviceProperties.DepthFormat, Aspect, m_DepthImage.View);
    MoveImageLayout<InitialLayout, DestinationLayout>(LogicalDevice, m_DepthImage.Image, DeviceProperties.DepthFormat, Queue, FamilyIndex);
}

std::vector<Object> BufferManager::AllocateScene(std::string_view const& ModelPath, VkDevice const& LogicalDevice, std::pair<std::uint8_t, VkQueue> const& QueuePair)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    std::vector<Object> Output;
    tinygltf::Model Model {};

    {
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
    }

    auto const TryResizeVertexContainer = [](std::vector<Vertex>& Vertices, std::uint32_t const NewSize) {
        if (std::size(Vertices) < NewSize && NewSize > 0U)
        {
            Vertices.resize(NewSize);
        }
    };

    auto const InsertIndiceInContainer = [](std::vector<std::uint32_t>& Indices, tinygltf::Accessor const& IndexAccessor, auto const* Data) {
        for (std::uint32_t Iterator = 0U; Iterator < static_cast<std::uint32_t>(IndexAccessor.count); ++Iterator)
        {
            Indices.push_back(static_cast<std::uint32_t>(Data[Iterator]));
        }
    };

    auto const AllocateModelTexture = [this, Model, LogicalDevice, QueuePair](ObjectAllocation& Allocation, std::int32_t const TextureIndex, TextureType const TextureType) {
        if (TextureIndex >= 0)
        {
            if (tinygltf::Texture const& Texture = Model.textures.at(TextureIndex);
                Texture.source >= 0)
            {
                tinygltf::Image const& Image = Model.images.at(Texture.source);

                Allocation.TextureImages.push_back(AllocateTexture(LogicalDevice, m_Allocator, QueuePair, std::data(Image.image), Image.width, Image.height, std::size(Image.image)));
                Allocation.TextureImages.back().Type = TextureType;
            }
        }
    };

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loaded model from path: '" << ModelPath << "'";
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loading scenes: " << std::size(Model.scenes);

    for (tinygltf::Node const& Node: Model.nodes)
    {
        std::int32_t const MeshIndex = Node.mesh;
        if (MeshIndex < 0)
        {
            continue;
        }

        tinygltf::Mesh const& Mesh = Model.meshes.at(MeshIndex);
        Output.reserve(std::size(Output) + std::size(Mesh.primitives));

        for (tinygltf::Primitive const& Primitive: Mesh.primitives)
        {
            std::uint32_t const ObjectID = m_ObjectIDCounter.fetch_add(1U);

            ObjectAllocation NewObjectAllocation {};
            std::vector<Vertex> NewVertices {};
            std::vector<std::uint32_t> NewIndices {};
            Object NewObject(ObjectID, ModelPath, std::format("{}_{:03d}", Mesh.name, ObjectID));

            {
                const float* PositionData {nullptr};
                const float* NormalData {nullptr};
                const float* TexCoordData {nullptr};

                if (Primitive.attributes.contains("POSITION"))
                {
                    tinygltf::Accessor const& PositionAccessor     = Model.accessors.at(Primitive.attributes.at("POSITION"));
                    tinygltf::BufferView const& PositionBufferView = Model.bufferViews.at(PositionAccessor.bufferView);
                    tinygltf::Buffer const& PositionBuffer         = Model.buffers.at(PositionBufferView.buffer);
                    PositionData                                   = reinterpret_cast<const float*>(std::data(PositionBuffer.data) + PositionBufferView.byteOffset + PositionAccessor.byteOffset);
                    TryResizeVertexContainer(NewVertices, PositionAccessor.count);
                }

                if (Primitive.attributes.contains("NORMAL"))
                {
                    tinygltf::Accessor const& NormalAccessor     = Model.accessors.at(Primitive.attributes.at("NORMAL"));
                    tinygltf::BufferView const& NormalBufferView = Model.bufferViews.at(NormalAccessor.bufferView);
                    tinygltf::Buffer const& NormalBuffer         = Model.buffers.at(NormalBufferView.buffer);
                    NormalData                                   = reinterpret_cast<const float*>(std::data(NormalBuffer.data) + NormalBufferView.byteOffset + NormalAccessor.byteOffset);
                    TryResizeVertexContainer(NewVertices, NormalAccessor.count);
                }

                if (Primitive.attributes.contains("TEXCOORD_0"))
                {
                    tinygltf::Accessor const& TexCoordAccessor     = Model.accessors.at(Primitive.attributes.at("TEXCOORD_0"));
                    tinygltf::BufferView const& TexCoordBufferView = Model.bufferViews.at(TexCoordAccessor.bufferView);
                    tinygltf::Buffer const& TexCoordBuffer         = Model.buffers.at(TexCoordBufferView.buffer);
                    TexCoordData                                   = reinterpret_cast<const float*>(std::data(TexCoordBuffer.data) + TexCoordBufferView.byteOffset + TexCoordAccessor.byteOffset);
                    TryResizeVertexContainer(NewVertices, TexCoordAccessor.count);
                }

                for (std::uint32_t Iterator = 0U; Iterator < static_cast<std::uint32_t>(std::size(NewVertices)); ++Iterator)
                {
                    if (PositionData)
                    {
                        NewVertices.at(Iterator).Position = glm::make_vec3(&PositionData[Iterator * 3]);
                    }

                    if (NormalData)
                    {
                        NewVertices.at(Iterator).Normal = glm::make_vec3(&NormalData[Iterator * 3]);
                    }

                    if (TexCoordData)
                    {
                        NewVertices.at(Iterator).TextureCoordinate = glm::make_vec2(&TexCoordData[Iterator * 2]);
                    }
                }
            }

            if (Primitive.indices >= 0)
            {
                tinygltf::Accessor const& IndexAccessor     = Model.accessors.at(Primitive.indices);
                tinygltf::BufferView const& IndexBufferView = Model.bufferViews.at(IndexAccessor.bufferView);
                tinygltf::Buffer const& IndexBuffer         = Model.buffers.at(IndexBufferView.buffer);
                unsigned char const* const IndicesData      = std::data(IndexBuffer.data) + IndexBufferView.byteOffset + IndexAccessor.byteOffset;

                NewIndices.reserve(IndexAccessor.count);
                NewObjectAllocation.IndicesCount = IndexAccessor.count;

                switch (IndexAccessor.componentType)
                {
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                        InsertIndiceInContainer(NewIndices, IndexAccessor, reinterpret_cast<const uint32_t*>(IndicesData));
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                        InsertIndiceInContainer(NewIndices, IndexAccessor, reinterpret_cast<const uint16_t*>(IndicesData));
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                        InsertIndiceInContainer(NewIndices, IndexAccessor, IndicesData);
                        break;
                    }
                    default:
                        break;
                }
            }

            if (!std::empty(Node.translation))
            {
                NewObject.SetPosition(Vector(glm::make_vec3(std::data(Node.translation))));
            }

            if (!std::empty(Node.scale))
            {
                NewObject.SetScale(Vector(glm::make_vec3(std::data(Node.scale))));
            }

            if (!std::empty(Node.rotation))
            {
                NewObject.SetRotation(Rotator(glm::make_quat(std::data(Node.rotation))));
            }

            if (Primitive.material >= 0)
            {
                tinygltf::Material const& Material = Model.materials.at(Primitive.material);
                AllocateModelTexture(NewObjectAllocation,
                                     Material.pbrMetallicRoughness.baseColorTexture.index,
                                     TextureType::BaseColor);

                if (std::empty(NewObjectAllocation.TextureImages))
                {
                    constexpr std::uint8_t DefaultTextureHalfSize {2U};
                    constexpr std::uint8_t DefaultTextureSize {DefaultTextureHalfSize * 2U};
                    constexpr std::array<std::uint8_t, DefaultTextureSize> DefaultTextureData {};

                    NewObjectAllocation.TextureImages.push_back(AllocateTexture(LogicalDevice,
                                                                                m_Allocator,
                                                                                QueuePair,
                                                                                std::data(DefaultTextureData),
                                                                                DefaultTextureHalfSize,
                                                                                DefaultTextureHalfSize,
                                                                                DefaultTextureSize));

                    NewObjectAllocation.TextureImages.back().Type = TextureType::BaseColor;
                }
            }

            CreateVertexBuffers(LogicalDevice, m_Allocator, QueuePair, NewObjectAllocation, NewVertices);
            CreateIndexBuffers(LogicalDevice, m_Allocator, QueuePair, NewObjectAllocation, NewIndices);
            CreateUniformBuffers(m_Allocator, NewObjectAllocation);

            m_Objects.emplace(NewObject.GetID(), std::move(NewObjectAllocation));
            Output.push_back(std::move(NewObject));
        }
    }

    return Output;
}

void BufferManager::ReleaseScene(VkDevice const& LogicalDevice, std::vector<std::uint32_t> const& ObjectIDs)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    for (std::uint32_t const ObjectIDIter: ObjectIDs)
    {
        if (!m_Objects.contains(ObjectIDIter))
        {
            return;
        }

        m_Objects.at(ObjectIDIter).DestroyResources(LogicalDevice, m_Allocator);
        m_Objects.erase(ObjectIDIter);
    }

    if (std::empty(m_Objects))
    {
        m_ObjectIDCounter = 0U;
    }
}

void BufferManager::ReleaseBufferResources(VkDevice const& LogicalDevice)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Releasing vulkan buffer resources";


    if (m_SwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(LogicalDevice, m_SwapChain, nullptr);
        m_SwapChain = VK_NULL_HANDLE;
    }

    DestroyBufferResources(LogicalDevice, true);

    vkDestroySurfaceKHR(volkGetLoadedInstance(), m_Surface, nullptr);
    m_Surface = VK_NULL_HANDLE;

    vmaDestroyAllocator(m_Allocator);
    m_Allocator = VK_NULL_HANDLE;
}

void BufferManager::DestroyBufferResources(VkDevice const& LogicalDevice, bool const ClearScene)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying vulkan buffer resources";


    for (ImageAllocation& ImageViewIter: m_SwapChainImages)
    {
        ImageViewIter.DestroyResources(LogicalDevice, m_Allocator);
    }
    m_SwapChainImages.clear();

    for (VkFramebuffer& FrameBufferIter: m_FrameBuffers)
    {
        if (FrameBufferIter != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(LogicalDevice, FrameBufferIter, nullptr);
            FrameBufferIter = VK_NULL_HANDLE;
        }
    }
    m_FrameBuffers.clear();

    m_DepthImage.DestroyResources(LogicalDevice, m_Allocator);

    if (ClearScene)
    {
        for (auto& ObjectIter: m_Objects | std::views::values)
        {
            ObjectIter.DestroyResources(LogicalDevice, m_Allocator);
        }
        m_Objects.clear();
    }
}

VkSurfaceKHR& BufferManager::GetSurface()
{
    return m_Surface;
}

VkSwapchainKHR const& BufferManager::GetSwapChain()
{
    return m_SwapChain;
}

VkExtent2D const& BufferManager::GetSwapChainExtent()
{
    return m_SwapChainExtent;
}

std::vector<VkFramebuffer> const& BufferManager::GetFrameBuffers()
{
    return m_FrameBuffers;
}

VkBuffer BufferManager::GetVertexBuffer(std::uint32_t const ObjectID)
{
    if (!m_Objects.contains(ObjectID))
    {
        return VK_NULL_HANDLE;
    }

    return m_Objects.at(ObjectID).VertexBuffer.Buffer;
}

VkBuffer BufferManager::GetIndexBuffer(std::uint32_t const ObjectID)
{
    if (!m_Objects.contains(ObjectID))
    {
        return VK_NULL_HANDLE;
    }

    return m_Objects.at(ObjectID).IndexBuffer.Buffer;
}

std::uint32_t BufferManager::GetIndicesCount(std::uint32_t const ObjectID)
{
    if (!m_Objects.contains(ObjectID))
    {
        return 0U;
    }

    return m_Objects.at(ObjectID).IndicesCount;
}

void* BufferManager::GetUniformData(std::uint32_t const ObjectID)
{
    if (!m_Objects.contains(ObjectID))
    {
        return nullptr;
    }

    return m_Objects.at(ObjectID).UniformBuffer.MappedData;
}

bool BufferManager::ContainsObject(std::uint32_t const ID) const
{
    return m_Objects.contains(ID);
}

std::vector<RenderCore::MeshBufferData> BufferManager::GetAllocatedObjects()
{
    std::vector<MeshBufferData> Output;
    for (auto const& [ID, Data]: m_Objects)
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

std::uint32_t BufferManager::GetNumAllocations() const
{
    return static_cast<std::uint32_t>(std::size(m_Objects));
}

std::uint32_t BufferManager::GetClampedNumAllocations() const
{
    return std::clamp(static_cast<std::uint32_t>(std::size(m_Objects)), 1U, UINT32_MAX);
}

void BufferManager::UpdateUniformBuffers(std::shared_ptr<Object> const& Object, VkExtent2D const& Extent)
{
    if (!Object)
    {
        return;
    }

    if (void* UniformBufferData = GetUniformData(Object->GetID()))
    {
        UniformBufferObject const UpdatedUBO {
                .Model      = Object->GetMatrix(),
                .View       = GetViewportCamera().GetViewMatrix(),
                .Projection = GetViewportCamera().GetProjectionMatrix(Extent)};

        std::memcpy(UniformBufferData, &UpdatedUBO, sizeof(UniformBufferObject));
    }
}

VmaAllocator const& BufferManager::GetAllocator()
{
    return m_Allocator;
}