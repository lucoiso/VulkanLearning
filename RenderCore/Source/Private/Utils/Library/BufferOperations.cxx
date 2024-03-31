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
#include <vma/vk_mem_alloc.h>
#include "Utils/Library/Macros.h"

module RenderCore.Runtime.Buffer.Operations;

import RuntimeInfo.Manager;
import RenderCore.Utils.Constants;

using namespace RenderCore;

VmaAllocationInfo RenderCore::CreateBuffer(VmaAllocator const         &Allocator,
                                           VkDeviceSize const         &Size,
                                           VkBufferUsageFlags const    Usage,
                                           VkMemoryPropertyFlags const Flags,
                                           std::string_view const      Identifier,
                                           VkBuffer                   &Buffer,
                                           VmaAllocation              &Allocation)
{
    PUSH_CALLSTACK();

    if (Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    VmaAllocationCreateInfo const AllocationCreateInfo {.flags = Flags, .usage = VMA_MEMORY_USAGE_AUTO};

    VkBufferCreateInfo const BufferCreateInfo {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                               .size  = std::clamp(Size, g_BufferMemoryAllocationSize, UINT64_MAX),
                                               .usage = Usage};

    VmaAllocationInfo MemoryAllocationInfo;
    CheckVulkanResult(vmaCreateBuffer(Allocator, &BufferCreateInfo, &AllocationCreateInfo, &Buffer, &Allocation, &MemoryAllocationInfo));

    vmaSetAllocationName(Allocator, Allocation, std::format("Buffer: {}", Identifier).c_str());

    return MemoryAllocationInfo;
}

void RenderCore::CopyBuffer(VkCommandBuffer const &CommandBuffer, VkBuffer const &Source, VkBuffer const &Destination, VkDeviceSize const &Size)
{
    PUSH_CALLSTACK();

    VkBufferCopy const BufferCopy {
        .size = Size,
    };

    vkCmdCopyBuffer(CommandBuffer, Source, Destination, 1U, &BufferCopy);
}

CommandBufferSet RenderCore::CreateVertexBuffers(VmaAllocator const &Allocator, ObjectAllocationData &Object, std::vector<Vertex> const &Vertices)
{
    PUSH_CALLSTACK();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating Vulkan vertex buffers";

    if (Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    VkDeviceSize const BufferSize = std::size(Vertices) * sizeof(Vertex);

    constexpr VkBufferUsageFlags    SourceUsageFlags               = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags      = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    constexpr VkBufferUsageFlags    DestinationUsageFlags          = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = 0U;

    CommandBufferSet Output {.bVertexBuffer = true, .AllocationSize = BufferSize};

    VmaAllocationInfo const StagingInfo = CreateBuffer(Allocator,
                                                       BufferSize,
                                                       SourceUsageFlags,
                                                       SourceMemoryPropertyFlags,
                                                       "STAGING_VERTEX",
                                                       Output.StagingBuffer.first,
                                                       Output.StagingBuffer.second);

    std::memcpy(StagingInfo.pMappedData, std::data(Vertices), BufferSize);

    CreateBuffer(Allocator,
                 BufferSize,
                 DestinationUsageFlags,
                 DestinationMemoryPropertyFlags,
                 "VERTEX",
                 Object.VertexBufferAllocation.Buffer,
                 Object.VertexBufferAllocation.Allocation);

    return Output;
}

CommandBufferSet RenderCore::CreateIndexBuffers(VmaAllocator const &Allocator, ObjectAllocationData &Object, std::vector<std::uint32_t> const &Indices)
{
    PUSH_CALLSTACK();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating Vulkan index buffers";

    if (Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    VkDeviceSize const BufferSize = std::size(Indices) * sizeof(std::uint32_t);

    constexpr VkBufferUsageFlags    SourceUsageFlags               = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags      = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    constexpr VkBufferUsageFlags    DestinationUsageFlags          = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = 0U;

    CommandBufferSet Output {.bVertexBuffer = false, .AllocationSize = BufferSize};

    VmaAllocationInfo const StagingInfo = CreateBuffer(Allocator,
                                                       BufferSize,
                                                       SourceUsageFlags,
                                                       SourceMemoryPropertyFlags,
                                                       "STAGING_INDEX",
                                                       Output.StagingBuffer.first,
                                                       Output.StagingBuffer.second);

    std::memcpy(StagingInfo.pMappedData, std::data(Indices), BufferSize);
    CheckVulkanResult(vmaFlushAllocation(Allocator, Output.StagingBuffer.second, 0U, BufferSize));

    CreateBuffer(Allocator,
                 BufferSize,
                 DestinationUsageFlags,
                 DestinationMemoryPropertyFlags,
                 "INDEX",
                 Object.IndexBufferAllocation.Buffer,
                 Object.IndexBufferAllocation.Allocation);

    return Output;
}

void RenderCore::CreateUniformBuffers(VmaAllocator const    &Allocator,
                                      BufferAllocation      &BufferAllocation,
                                      VkDeviceSize const     BufferSize,
                                      std::string_view const Identifier)
{
    if (Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    constexpr VkBufferUsageFlags    DestinationUsageFlags          = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    CreateBuffer(Allocator,
                 BufferSize,
                 DestinationUsageFlags,
                 DestinationMemoryPropertyFlags,
                 Identifier,
                 BufferAllocation.Buffer,
                 BufferAllocation.Allocation);

    vmaMapMemory(Allocator, BufferAllocation.Allocation, &BufferAllocation.MappedData);
}

void RenderCore::CreateModelUniformBuffers(VmaAllocator const &Allocator, ObjectAllocationData &Object)
{
    PUSH_CALLSTACK();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating Vulkan model uniform buffers";

    constexpr VkDeviceSize BufferSize = sizeof(glm::mat4);
    CreateUniformBuffers(Allocator, Object.UniformBufferAllocation, BufferSize, "MODEL_UNIFORM");
    Object.ModelDescriptors.push_back(VkDescriptorBufferInfo {.buffer = Object.UniformBufferAllocation.Buffer, .offset = 0U, .range = BufferSize});
}

void RenderCore::CreateImage(VmaAllocator const            &Allocator,
                             VkFormat const                &ImageFormat,
                             VkExtent2D const              &Extent,
                             VkImageTiling const           &Tiling,
                             VkImageUsageFlags const        ImageUsage,
                             VmaAllocationCreateFlags const Flags,
                             VmaMemoryUsage const           MemoryUsage,
                             std::string_view const         Identifier,
                             VkImage                       &Image,
                             VmaAllocation                 &Allocation)
{
    PUSH_CALLSTACK();

    if (Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    VkImageCreateInfo const ImageViewCreateInfo {.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                                                 .imageType     = VK_IMAGE_TYPE_2D,
                                                 .format        = ImageFormat,
                                                 .extent        = {.width = Extent.width, .height = Extent.height, .depth = 1U},
                                                 .mipLevels     = 1U,
                                                 .arrayLayers   = 1U,
                                                 .samples       = g_MSAASamples,
                                                 .tiling        = Tiling,
                                                 .usage         = ImageUsage,
                                                 .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
                                                 .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};

    VmaAllocationCreateInfo const ImageCreateInfo {.flags = Flags, .usage = MemoryUsage};

    VmaAllocationInfo AllocationInfo;
    CheckVulkanResult(vmaCreateImage(Allocator, &ImageViewCreateInfo, &ImageCreateInfo, &Image, &Allocation, &AllocationInfo));

    vmaSetAllocationName(Allocator, Allocation, std::format("Image: {}", Identifier).c_str());
}

void RenderCore::CreateTextureSampler(VkPhysicalDevice const &Device, VkSampler &Sampler)
{
    PUSH_CALLSTACK();

    if (Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    VkPhysicalDeviceProperties SurfaceProperties;
    vkGetPhysicalDeviceProperties(Device, &SurfaceProperties);

    VkSamplerCreateInfo const SamplerCreateInfo {.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                                                 .magFilter               = VK_FILTER_LINEAR,
                                                 .minFilter               = VK_FILTER_LINEAR,
                                                 .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                                                 .addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                                 .addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                                 .addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                                                 .mipLodBias              = 0.F,
                                                 .anisotropyEnable        = VK_FALSE,
                                                 .maxAnisotropy           = SurfaceProperties.limits.maxSamplerAnisotropy,
                                                 .compareEnable           = VK_FALSE,
                                                 .compareOp               = VK_COMPARE_OP_ALWAYS,
                                                 .minLod                  = 0.F,
                                                 .maxLod                  = FLT_MAX,
                                                 .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                                                 .unnormalizedCoordinates = VK_FALSE};

    CheckVulkanResult(vkCreateSampler(volkGetLoadedDevice(), &SamplerCreateInfo, nullptr, &Sampler));
}

void RenderCore::CreateImageView(VkImage const &Image, VkFormat const &Format, VkImageAspectFlags const &AspectFlags, VkImageView &ImageView)
{
    PUSH_CALLSTACK();

    VkImageViewCreateInfo const ImageViewCreateInfo {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image            = Image,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = Format,
        .subresourceRange = {.aspectMask = AspectFlags, .baseMipLevel = 0U, .levelCount = 1U, .baseArrayLayer = 0U, .layerCount = 1U}};

    CheckVulkanResult(vkCreateImageView(volkGetLoadedDevice(), &ImageViewCreateInfo, nullptr, &ImageView));
}

void RenderCore::CreateSwapChainImageViews(std::vector<ImageAllocation> &Images, VkFormat const ImageFormat)
{
    PUSH_CALLSTACK_WITH_COUNTER();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating vulkan swap chain image views";

    std::ranges::for_each(Images,
                          [&](ImageAllocation &ImageIter)
                          {
                              CreateImageView(ImageIter.Image, ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, ImageIter.View);
                          });
}

void RenderCore::CreateTextureImageView(ImageAllocation &Allocation, VkFormat const ImageFormat)
{
    PUSH_CALLSTACK();
    CreateImageView(Allocation.Image, ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, Allocation.View);
}

void RenderCore::CopyBufferToImage(VkCommandBuffer const &CommandBuffer, VkBuffer const &Source, VkImage const &Destination, VkExtent2D const &Extent)
{
    PUSH_CALLSTACK();

    VkBufferImageCopy const BufferImageCopy {.bufferOffset      = 0U,
                                             .bufferRowLength   = 0U,
                                             .bufferImageHeight = 0U,
                                             .imageSubresource
                                             = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0U, .baseArrayLayer = 0U, .layerCount = 1U},
                                             .imageOffset = {.x = 0U, .y = 0U, .z = 0U},
                                             .imageExtent = {.width = Extent.width, .height = Extent.height, .depth = 1U}};

    vkCmdCopyBufferToImage(CommandBuffer, Source, Destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &BufferImageCopy);
}

ImageCreationData RenderCore::AllocateTexture(VmaAllocator const  &Allocator,
                                              unsigned char const *Data,
                                              std::uint32_t const  Width,
                                              std::uint32_t const  Height,
                                              VkFormat const       ImageFormat,
                                              std::size_t const    AllocationSize)
{
    PUSH_CALLSTACK();

    ImageAllocation ImageAllocation {};

    constexpr VmaMemoryUsage MemoryUsage = VMA_MEMORY_USAGE_AUTO;

    constexpr VkBufferUsageFlags       SourceUsageFlags          = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VmaAllocationCreateFlags SourceMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    constexpr VkImageUsageFlags        DestinationUsageFlags          = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    constexpr VmaAllocationCreateFlags DestinationMemoryPropertyFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    constexpr VkImageTiling Tiling = VK_IMAGE_TILING_LINEAR;

    ImageCreationData Output {};

    VmaAllocationInfo const StagingInfo = CreateBuffer(Allocator,
                                                       std::clamp(AllocationSize, g_ImageBufferMemoryAllocationSize, UINT64_MAX),
                                                       SourceUsageFlags,
                                                       SourceMemoryPropertyFlags,
                                                       "STAGING_TEXTURE",
                                                       Output.StagingBuffer.first,
                                                       Output.StagingBuffer.second);

    std::memcpy(StagingInfo.pMappedData, Data, AllocationSize);

    VkExtent2D const Extent {.width = Width, .height = Height};

    CreateImage(Allocator,
                ImageFormat,
                Extent,
                Tiling,
                DestinationUsageFlags,
                DestinationMemoryPropertyFlags,
                MemoryUsage,
                "TEXTURE",
                ImageAllocation.Image,
                ImageAllocation.Allocation);

    Output.Allocation = ImageAllocation;
    Output.Format     = ImageFormat;
    Output.Extent     = Extent;

    return Output;
}
