// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <Volk/volk.h>
#include <glm/matrix.hpp>
#include <ranges>
#include <stb_image_write.h>

#ifndef VMA_IMPLEMENTATION
#include <boost/log/trivial.hpp>
#define VMA_LEAK_LOG_FORMAT(format, ...)                                       \
        do                                                                     \
        {                                                                      \
            std::size_t _BuffSize = snprintf(nullptr, 0, format, __VA_ARGS__); \
            std::string _Message(_BuffSize + 1, '\0');                         \
            snprintf(&_Message[0], _Message.size(), format, __VA_ARGS__);      \
            _Message.pop_back();                                               \
            BOOST_LOG_TRIVIAL(debug) << _Message;                              \
        }                                                                      \
        while (false)
#define VMA_IMPLEMENTATION
#endif
#include <vma/vk_mem_alloc.h>

module RenderCore.Runtime.Memory;

import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import RenderCore.Runtime.Command;
import RenderCore.Runtime.Device;
import RenderCore.Types.Object;
import RenderCore.Types.UniformBufferObject;

using namespace RenderCore;

VmaPool g_StagingBufferPool { VK_NULL_HANDLE };
VmaPool g_BufferPool { VK_NULL_HANDLE };
VmaPool g_ImagePool { VK_NULL_HANDLE };
VmaAllocator g_Allocator { VK_NULL_HANDLE };

void RenderCore::CreateMemoryAllocator(VkPhysicalDevice const &PhysicalDevice)
{
    VmaVulkanFunctions const VulkanFunctions { .vkGetInstanceProcAddr = vkGetInstanceProcAddr, .vkGetDeviceProcAddr = vkGetDeviceProcAddr };

    VmaAllocatorCreateInfo const AllocatorInfo {
            .flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT | VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT,
            .physicalDevice = PhysicalDevice,
            .device = volkGetLoadedDevice(),
            .preferredLargeHeapBlockSize = 0U /*Default: 256 MiB*/,
            .pAllocationCallbacks = nullptr,
            .pDeviceMemoryCallbacks = nullptr,
            .pHeapSizeLimit = nullptr,
            .pVulkanFunctions = &VulkanFunctions,
            .instance = volkGetLoadedInstance(),
            .vulkanApiVersion = VK_API_VERSION_1_3,
            .pTypeExternalMemoryHandleTypes = nullptr
    };

    CheckVulkanResult(vmaCreateAllocator(&AllocatorInfo, &g_Allocator));

    {
        constexpr VmaAllocationCreateInfo AllocationCreateInfo {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST
        };

        constexpr VkBufferCreateInfo BufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = 0x100,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        };

        std::uint32_t MemoryType;
        CheckVulkanResult(vmaFindMemoryTypeIndexForBufferInfo(g_Allocator, &BufferCreateInfo, &AllocationCreateInfo, &MemoryType));

        VmaPoolCreateInfo const PoolCreateInfo {
            .memoryTypeIndex = MemoryType,
            .flags = VMA_POOL_CREATE_LINEAR_ALGORITHM_BIT,
            .blockSize = g_ImageMemoryAllocationSize,
            .minBlockCount = g_MinStagingMemoryBlock,
            .priority = 0.F,
            .minAllocationAlignment = GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment
        };

        CheckVulkanResult(vmaCreatePool(g_Allocator, &PoolCreateInfo, &g_StagingBufferPool));
    }

    {
        constexpr VmaAllocationCreateInfo AllocationCreateInfo {
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
        };

        constexpr VkBufferCreateInfo BufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = 0x100,
            .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
        };

        std::uint32_t MemoryType;
        CheckVulkanResult(vmaFindMemoryTypeIndexForBufferInfo(g_Allocator, &BufferCreateInfo, &AllocationCreateInfo, &MemoryType));

        VmaPoolCreateInfo const PoolCreateInfo {
            .memoryTypeIndex = MemoryType,
            .flags = VMA_POOL_CREATE_LINEAR_ALGORITHM_BIT,
            .blockSize = g_BufferMemoryAllocationSize,
            .minBlockCount = g_MinMemoryBlock,
            .priority = 1.F,
            .minAllocationAlignment = GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment
        };

        CheckVulkanResult(vmaCreatePool(g_Allocator, &PoolCreateInfo, &g_BufferPool));
    }

    {
        constexpr VmaAllocationCreateInfo AllocationCreateInfo {
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
        };

        constexpr VkImageCreateInfo ImageViewCreateInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_R8G8B8A8_SRGB,
            .extent = { .width = 256U, .height = 256U, .depth = 1U },
            .mipLevels = 1U,
            .arrayLayers = 1U,
            .samples = g_MSAASamples,
            .tiling = VK_IMAGE_TILING_LINEAR,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };

        std::uint32_t MemoryType;
        CheckVulkanResult(vmaFindMemoryTypeIndexForImageInfo(g_Allocator, &ImageViewCreateInfo, &AllocationCreateInfo, &MemoryType));

        VmaPoolCreateInfo const PoolCreateInfo {
            .memoryTypeIndex = MemoryType,
            .flags = VMA_POOL_CREATE_LINEAR_ALGORITHM_BIT,
            .blockSize = g_ImageMemoryAllocationSize,
            .minBlockCount = g_MinMemoryBlock,
            .priority = 0.5F
        };

        CheckVulkanResult(vmaCreatePool(g_Allocator, &PoolCreateInfo, &g_ImagePool));
    }
}

void RenderCore::ReleaseMemoryResources()
{
    vmaDestroyPool(g_Allocator, g_StagingBufferPool);
    g_StagingBufferPool = VK_NULL_HANDLE;

    vmaDestroyPool(g_Allocator, g_BufferPool);
    g_BufferPool = VK_NULL_HANDLE;

    vmaDestroyPool(g_Allocator, g_ImagePool);
    g_ImagePool = VK_NULL_HANDLE;

    vmaDestroyAllocator(g_Allocator);
    g_Allocator = VK_NULL_HANDLE;
}

VmaAllocator const &RenderCore::GetAllocator()
{
    return g_Allocator;
}

VmaAllocationInfo RenderCore::CreateBuffer(VkDeviceSize const &        Size,
                                           VkBufferUsageFlags const    Usage,
                                           VkMemoryPropertyFlags const Flags,
                                           std::string_view const      Identifier,
                                           VkBuffer &                  Buffer,
                                           VmaAllocation &             Allocation)
{
    constexpr VkMemoryPropertyFlags StagingMemoryFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    bool const IsStaging = HasAnyFlag<VkMemoryPropertyFlags>(Flags, StagingMemoryFlags);

    VmaAllocationCreateInfo const AllocationCreateInfo {
        .flags = Flags,
        .usage = IsStaging ? VMA_MEMORY_USAGE_AUTO_PREFER_HOST : VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .pool = IsStaging ? g_StagingBufferPool : g_BufferPool
    };

    VkBufferCreateInfo const BufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = Size,
            .usage = Usage
    };

    VmaAllocationInfo MemoryAllocationInfo;
    CheckVulkanResult(vmaCreateBuffer(GetAllocator(), &BufferCreateInfo, &AllocationCreateInfo, &Buffer, &Allocation, &MemoryAllocationInfo));

    vmaSetAllocationName(GetAllocator(), Allocation, std::data(std::format("Buffer: {}", Identifier)));

    return MemoryAllocationInfo;
}

void RenderCore::CopyBuffer(VkCommandBuffer const &CommandBuffer, VkBuffer const &Source, VkBuffer const &Destination, VkDeviceSize const &Size)
{
    VkBufferCopy const BufferCopy { .size = Size, };
    vkCmdCopyBuffer(CommandBuffer, Source, Destination, 1U, &BufferCopy);
}

void RenderCore::CreateUniformBuffers(BufferAllocation &BufferAllocation, VkDeviceSize const BufferSize, std::string_view const Identifier)
{
    constexpr VkBufferUsageFlags    DestinationUsageFlags          = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    CreateBuffer(BufferSize, DestinationUsageFlags, DestinationMemoryPropertyFlags, Identifier, BufferAllocation.Buffer, BufferAllocation.Allocation);

    vmaMapMemory(GetAllocator(), BufferAllocation.Allocation, &BufferAllocation.MappedData);
}

void RenderCore::CreateImage(VkFormat const &               ImageFormat,
                             VkExtent2D const &             Extent,
                             VkImageTiling const &          Tiling,
                             VkImageUsageFlags const        ImageUsage,
                             VmaAllocationCreateFlags const Flags,
                             VmaMemoryUsage const           MemoryUsage,
                             std::string_view const         Identifier,
                             VkImage &                      Image,
                             VmaAllocation &                Allocation)
{
    VkImageCreateInfo const ImageViewCreateInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = ImageFormat,
            .extent = { .width = Extent.width, .height = Extent.height, .depth = 1U },
            .mipLevels = 1U,
            .arrayLayers = 1U,
            .samples = g_MSAASamples,
            .tiling = Tiling,
            .usage = ImageUsage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VmaAllocationCreateInfo const ImageCreateInfo {
        .flags = Flags,
        .usage = MemoryUsage,
        .pool = g_ImagePool
    };

    VmaAllocationInfo AllocationInfo;
    CheckVulkanResult(vmaCreateImage(GetAllocator(), &ImageViewCreateInfo, &ImageCreateInfo, &Image, &Allocation, &AllocationInfo));

    vmaSetAllocationName(GetAllocator(), Allocation, std::data(std::format("Image: {}", Identifier)));
}

void RenderCore::CreateTextureSampler(VkPhysicalDevice const &Device, VkSampler &Sampler)
{
    VkPhysicalDeviceProperties SurfaceProperties;
    vkGetPhysicalDeviceProperties(Device, &SurfaceProperties);

    VkSamplerCreateInfo const SamplerCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias = 0.F,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = SurfaceProperties.limits.maxSamplerAnisotropy,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .minLod = 0.F,
            .maxLod = FLT_MAX,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE
    };

    CheckVulkanResult(vkCreateSampler(volkGetLoadedDevice(), &SamplerCreateInfo, nullptr, &Sampler));
}

void RenderCore::CreateImageView(VkImage const &Image, VkFormat const &Format, VkImageAspectFlags const &AspectFlags, VkImageView &ImageView)
{
    VkImageViewCreateInfo const ImageViewCreateInfo {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = Image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = Format,
            .subresourceRange = { .aspectMask = AspectFlags, .baseMipLevel = 0U, .levelCount = 1U, .baseArrayLayer = 0U, .layerCount = 1U }
    };

    CheckVulkanResult(vkCreateImageView(volkGetLoadedDevice(), &ImageViewCreateInfo, nullptr, &ImageView));
}

void RenderCore::CreateTextureImageView(ImageAllocation &Allocation, VkFormat const ImageFormat)
{
    CreateImageView(Allocation.Image, ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, Allocation.View);
}

void RenderCore::CopyBufferToImage(VkCommandBuffer const &CommandBuffer, VkBuffer const &Source, VkImage const &Destination, VkExtent2D const &Extent)
{
    VkBufferImageCopy const BufferImageCopy {
            .bufferOffset = 0U,
            .bufferRowLength = 0U,
            .bufferImageHeight = 0U,
            .imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0U, .baseArrayLayer = 0U, .layerCount = 1U },
            .imageOffset = { .x = 0U, .y = 0U, .z = 0U },
            .imageExtent = { .width = Extent.width, .height = Extent.height, .depth = 1U }
    };

    vkCmdCopyBufferToImage(CommandBuffer, Source, Destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &BufferImageCopy);
}

std::pair<VkBuffer, VmaAllocation> RenderCore::AllocateTexture(VkCommandBuffer &    CommandBuffer,
                                                               unsigned char const *Data,
                                                               std::uint32_t const  Width,
                                                               std::uint32_t const  Height,
                                                               VkFormat const       ImageFormat,
                                                               std::size_t const    AllocationSize,
                                                               ImageAllocation &    ImageAllocation)
{
    constexpr VkBufferUsageFlags       SourceUsageFlags          = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VmaAllocationCreateFlags SourceMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    constexpr VkImageUsageFlags        DestinationUsageFlags          = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    constexpr VmaAllocationCreateFlags DestinationMemoryPropertyFlags = 0U;

    constexpr VmaMemoryUsage MemoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    constexpr VkImageTiling Tiling = VK_IMAGE_TILING_LINEAR;

    std::pair<VkBuffer, VmaAllocation> Output;
    VmaAllocationInfo const            StagingInfo = CreateBuffer(AllocationSize,
                                                                  SourceUsageFlags,
                                                                  SourceMemoryPropertyFlags,
                                                                  "STAGING_TEXTURE",
                                                                  Output.first,
                                                                  Output.second);

    std::memcpy(StagingInfo.pMappedData, Data, AllocationSize);

    ImageAllocation.Extent = { .width = Width, .height = Height };
    ImageAllocation.Format = ImageFormat;

    CreateImage(ImageFormat,
                ImageAllocation.Extent,
                Tiling,
                DestinationUsageFlags,
                DestinationMemoryPropertyFlags,
                MemoryUsage,
                "TEXTURE",
                ImageAllocation.Image,
                ImageAllocation.Allocation);

    MoveImageLayout<VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT>(CommandBuffer,
        ImageAllocation.Image,
        ImageAllocation.Format);

    CopyBufferToImage(CommandBuffer, Output.first, ImageAllocation.Image, ImageAllocation.Extent);

    MoveImageLayout<VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR, VK_IMAGE_ASPECT_COLOR_BIT>(CommandBuffer,
        ImageAllocation.Image,
        ImageAllocation.Format);

    CreateImageView(ImageAllocation.Image, ImageAllocation.Format, VK_IMAGE_ASPECT_COLOR_BIT, ImageAllocation.View);

    return Output;
}

std::pair<VkBuffer, VmaAllocation> RenderCore::AllocateModelBuffers(VkCommandBuffer const &CommandBuffer, Object &Object)
{
    ObjectAllocationData &            ObjectAllocation = Object.GetMutableAllocationData();
    std::vector<Vertex> const &       Vertices         = Object.GetVertices();
    std::vector<std::uint32_t> const &Indices          = Object.GetIndices();
    std::size_t const                 AllocationSize   = Object.GetAllocationSize();

    constexpr VkBufferUsageFlags    SourceUsageFlags          = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    constexpr VkBufferUsageFlags    DestinationUsageFlags     = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                                                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    std::pair<VkBuffer, VmaAllocation> Output;

    VmaAllocationInfo StagingInfo = CreateBuffer(AllocationSize,
                                                 SourceUsageFlags,
                                                 SourceMemoryPropertyFlags,
                                                 "STAGING_MODEL_UNIFIED_BUFFER",
                                                 Output.first,
                                                 Output.second);

    CheckVulkanResult(vmaMapMemory(GetAllocator(), Output.second, &StagingInfo.pMappedData));
    std::memcpy(static_cast<char *>(StagingInfo.pMappedData) + Object.GetVertexBufferStart(), std::data(Vertices), Object.GetVertexBufferSize());
    std::memcpy(static_cast<char *>(StagingInfo.pMappedData) + Object.GetIndexBufferStart(), std::data(Indices), Object.GetIndexBufferSize());
    CheckVulkanResult(vmaFlushAllocation(GetAllocator(), Output.second, Object.GetIndexBufferStart(), Object.GetIndexBufferSize()));

    CreateBuffer(AllocationSize,
                 DestinationUsageFlags,
                 DestinationMemoryPropertyFlags,
                 "MODEL_UNIFIED_BUFFER",
                 ObjectAllocation.BufferAllocation.Buffer,
                 ObjectAllocation.BufferAllocation.Allocation);

    VkBufferCopy const BufferCopy { .size = AllocationSize };
    vkCmdCopyBuffer(CommandBuffer, Output.first, ObjectAllocation.BufferAllocation.Buffer, 1U, &BufferCopy);

    vmaUnmapMemory(GetAllocator(), Output.second);
    CheckVulkanResult(vmaMapMemory(GetAllocator(), ObjectAllocation.BufferAllocation.Allocation, &ObjectAllocation.BufferAllocation.MappedData));

    ObjectAllocation.ModelDescriptors = {
            {
                    UniformType::Model,
                    VkDescriptorBufferInfo {
                            .buffer = ObjectAllocation.BufferAllocation.Buffer,
                            .offset = Object.GetModelUniformStart(),
                            .range = Object.GetModelUniformBufferSize()
                    }
            },
            {
                    UniformType::Material,
                    VkDescriptorBufferInfo {
                            .buffer = ObjectAllocation.BufferAllocation.Buffer,
                            .offset = Object.GetMaterialUniformStart(),
                            .range = Object.GetMaterialUniformBufferSize()
                    }
            }
    };

    return Output;
}

void RenderCore::SaveImageToFile(VkImage const &Image, std::string_view const Path, VkExtent2D const &Extent)
{
    VkBuffer            Buffer;
    VmaAllocation       Allocation;
    VkSubresourceLayout Layout;

    constexpr std::uint8_t Components { 4U };

    auto const &[FamilyIndex, Queue] = GetGraphicsQueue();

    VkCommandPool                CommandPool { VK_NULL_HANDLE };
    std::vector<VkCommandBuffer> CommandBuffer { VK_NULL_HANDLE };
    InitializeSingleCommandQueue(CommandPool, CommandBuffer, FamilyIndex);
    {
        VkImageSubresource SubResource { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .arrayLayer = 0 };

        vkGetImageSubresourceLayout(volkGetLoadedDevice(), Image, &SubResource, &Layout);

        VkBufferCreateInfo BufferInfo {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = Layout.size,
                .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        VmaAllocationCreateInfo AllocationInfo { .usage = VMA_MEMORY_USAGE_CPU_ONLY };

        vmaCreateBuffer(g_Allocator, &BufferInfo, &AllocationInfo, &Buffer, &Allocation, nullptr);

        VkBufferImageCopy Region {
                .bufferOffset = 0U,
                .bufferRowLength = 0U,
                .bufferImageHeight = 0U,
                .imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0U, .baseArrayLayer = 0U, .layerCount = 1U },
                .imageOffset = { 0U, 0U, 0U },
                .imageExtent = { .width = Extent.width, .height = Extent.height, .depth = 1U }
        };

        VkImageMemoryBarrier2 PreCopyBarrier {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE_KHR,
                .srcAccessMask = VK_ACCESS_2_NONE_KHR,
                .dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
                .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = Image,
                .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0U,
                        .levelCount = 1U,
                        .baseArrayLayer = 0U,
                        .layerCount = 1U
                }
        };

        VkImageMemoryBarrier2 PostCopyBarrier {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
                .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_NONE_KHR,
                .dstAccessMask = VK_ACCESS_2_NONE_KHR,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = Image,
                .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0U,
                        .levelCount = 1U,
                        .baseArrayLayer = 0U,
                        .layerCount = 1U
                }
        };

        VkDependencyInfo DependencyInfo {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
                .imageMemoryBarrierCount = 1U,
                .pImageMemoryBarriers = &PreCopyBarrier
        };

        vkCmdPipelineBarrier2(CommandBuffer.back(), &DependencyInfo);
        vkCmdCopyImageToBuffer(CommandBuffer.back(), Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Buffer, 1U, &Region);

        DependencyInfo.pImageMemoryBarriers = &PostCopyBarrier;
        vkCmdPipelineBarrier2(CommandBuffer.back(), &DependencyInfo);
    }
    FinishSingleCommandQueue(Queue, CommandPool, CommandBuffer);

    void *ImageData;
    vmaMapMemory(g_Allocator, Allocation, &ImageData);

    auto ImagePixels = static_cast<unsigned char *>(ImageData);

    stbi_write_png(std::data(Path), Extent.width, Extent.height, Components, ImagePixels, Extent.width * Components);

    vmaUnmapMemory(g_Allocator, Allocation);
    vmaDestroyBuffer(g_Allocator, Buffer, Allocation);
}
