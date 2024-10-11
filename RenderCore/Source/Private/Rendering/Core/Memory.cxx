// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <stb_image_write.h>

#ifndef VMA_IMPLEMENTATION
#define VMA_LEAK_LOG_FORMAT(format, ...)                                            \
        do                                                                          \
        {                                                                           \
            std::size_t      _BuffSize = snprintf(nullptr, 0, format, __VA_ARGS__); \
            strzilla::string _Message(_BuffSize + 1, '\0');                         \
            snprintf(&_Message[0], _Message.size(), format, __VA_ARGS__);           \
            _Message.pop_back();                                                    \
            BOOST_LOG_TRIVIAL(debug) << _Message;                                   \
        }                                                                           \
        while (false)

#define VMA_IMPLEMENTATION
#endif
#include <vma/vk_mem_alloc.h>

module RenderCore.Runtime.Memory;

import RenderCore.Runtime.Device;
import RenderCore.Runtime.Instance;
import RenderCore.Runtime.Command;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Types.Vertex;
import RenderCore.Types.Material;
import RenderCore.Types.Mesh;
import RenderCore.Utils.Helpers;

using namespace RenderCore;

void RenderCore::CreateMemoryAllocator()
{
    VkPhysicalDevice const &PhysicalDevice = GetPhysicalDevice();
    VkDevice const &        LogicalDevice  = GetLogicalDevice();

    VmaVulkanFunctions const VulkanFunctions { .vkGetInstanceProcAddr = vkGetInstanceProcAddr, .vkGetDeviceProcAddr = vkGetDeviceProcAddr };

    VmaAllocatorCreateInfo const AllocatorInfo {
            .flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT | VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT |
                     VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
            .physicalDevice = PhysicalDevice,
            .device = LogicalDevice,
            .preferredLargeHeapBlockSize = 0U /*Default: 256 MiB*/,
            .pAllocationCallbacks = nullptr,
            .pDeviceMemoryCallbacks = nullptr,
            .pHeapSizeLimit = nullptr,
            .pVulkanFunctions = &VulkanFunctions,
            .instance = GetInstance(),
            .vulkanApiVersion = VK_API_VERSION_1_3,
            .pTypeExternalMemoryHandleTypes = nullptr
    };

    CheckVulkanResult(vmaCreateAllocator(&AllocatorInfo, &g_Allocator));

    auto const &Limits = GetPhysicalDeviceProperties().limits;

    {
        // Staging Uniform Buffer Pool
        constexpr VmaAllocationCreateInfo AllocationCreateInfo { .flags = g_MapMemoryFlag, .usage = g_StagingMemoryUsage };

        constexpr VkBufferCreateInfo BufferCreateInfo { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = 0x100, .usage = g_UniformBufferUsage };

        std::uint32_t MemoryType;
        CheckVulkanResult(vmaFindMemoryTypeIndexForBufferInfo(g_Allocator, &BufferCreateInfo, &AllocationCreateInfo, &MemoryType));

        VmaPoolCreateInfo const PoolCreateInfo {
                .memoryTypeIndex = MemoryType,
                .flags = VMA_POOL_CREATE_LINEAR_ALGORITHM_BIT,
                .minBlockCount = 0U,
                .priority = 0.F,
                .minAllocationAlignment = Limits.minUniformBufferOffsetAlignment
        };

        CheckVulkanResult(vmaCreatePool(g_Allocator, &PoolCreateInfo, &g_StagingUniformBufferPool));
        vmaSetPoolName(g_Allocator, g_StagingUniformBufferPool, "Staging Uniform Buffer Pool");
    }

    {
        // Staging Storage Buffer Pool
        constexpr VmaAllocationCreateInfo AllocationCreateInfo{ .flags = g_MapMemoryFlag, .usage = g_StagingMemoryUsage };

        constexpr VkBufferCreateInfo BufferCreateInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = 0x100, .usage = g_StorageBufferUsage };

        std::uint32_t MemoryType;
        CheckVulkanResult(vmaFindMemoryTypeIndexForBufferInfo(g_Allocator, &BufferCreateInfo, &AllocationCreateInfo, &MemoryType));

        VmaPoolCreateInfo const PoolCreateInfo{
                .memoryTypeIndex = MemoryType,
                .flags = VMA_POOL_CREATE_LINEAR_ALGORITHM_BIT,
                .minBlockCount = 0U,
                .priority = 0.F,
                .minAllocationAlignment = Limits.minStorageBufferOffsetAlignment
        };

        CheckVulkanResult(vmaCreatePool(g_Allocator, &PoolCreateInfo, &g_StagingStorageBufferPool));
        vmaSetPoolName(g_Allocator, g_StagingStorageBufferPool, "Staging Storage Buffer Pool");
    }

    {
        // Descriptor Buffer Pool
        constexpr VmaAllocationCreateInfo AllocationCreateInfo { .flags = g_MapMemoryFlag, .usage = g_DescriptorMemoryUsage };

        constexpr VkBufferCreateInfo BufferCreateInfo {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = 0x100,
                .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        };

        std::uint32_t MemoryType;
        CheckVulkanResult(vmaFindMemoryTypeIndexForBufferInfo(g_Allocator, &BufferCreateInfo, &AllocationCreateInfo, &MemoryType));

        VmaPoolCreateInfo const PoolCreateInfo {
                .memoryTypeIndex = MemoryType,
                .flags = VMA_POOL_CREATE_LINEAR_ALGORITHM_BIT,
                .minBlockCount = 0U,
                .priority = 1.F
        };

        CheckVulkanResult(vmaCreatePool(g_Allocator, &PoolCreateInfo, &g_DescriptorBufferPool));
        vmaSetPoolName(g_Allocator, g_DescriptorBufferPool, "Descriptor Buffer Pool");
    }

    {
        // Uniform Buffer Pool
        constexpr VmaAllocationCreateInfo AllocationCreateInfo { .flags = g_MapMemoryFlag, .usage = g_ModelMemoryUsage };

        constexpr VkBufferCreateInfo BufferCreateInfo { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = 0x100, .usage = g_UniformBufferUsage };

        std::uint32_t MemoryType;
        CheckVulkanResult(vmaFindMemoryTypeIndexForBufferInfo(g_Allocator, &BufferCreateInfo, &AllocationCreateInfo, &MemoryType));

        VmaPoolCreateInfo const PoolCreateInfo {
                .memoryTypeIndex = MemoryType,
                .flags = VMA_POOL_CREATE_LINEAR_ALGORITHM_BIT,
                .priority = 1.F,
                .minAllocationAlignment = Limits.minUniformBufferOffsetAlignment
        };

        CheckVulkanResult(vmaCreatePool(g_Allocator, &PoolCreateInfo, &g_UniformPool));
        vmaSetPoolName(g_Allocator, g_UniformPool, "Uniform Buffer Pool");
    }

    {
        // Storage Buffer Pool
        constexpr VmaAllocationCreateInfo AllocationCreateInfo{ .flags = g_MapMemoryFlag, .usage = g_ModelMemoryUsage };

        constexpr VkBufferCreateInfo BufferCreateInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = 0x100, .usage = g_StorageBufferUsage };

        std::uint32_t MemoryType;
        CheckVulkanResult(vmaFindMemoryTypeIndexForBufferInfo(g_Allocator, &BufferCreateInfo, &AllocationCreateInfo, &MemoryType));

        VmaPoolCreateInfo const PoolCreateInfo{
                .memoryTypeIndex = MemoryType,
                .flags = VMA_POOL_CREATE_LINEAR_ALGORITHM_BIT,
                .priority = 1.F,
                .minAllocationAlignment = Limits.minStorageBufferOffsetAlignment
        };

        CheckVulkanResult(vmaCreatePool(g_Allocator, &PoolCreateInfo, &g_StoragePool));
        vmaSetPoolName(g_Allocator, g_StoragePool, "Storage Buffer Pool");
    }

    {
        // Image Pool
        constexpr VmaAllocationCreateInfo AllocationCreateInfo { .usage = g_TextureMemoryUsage };

        constexpr VkImageCreateInfo ImageViewCreateInfo {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = VK_FORMAT_R8G8B8A8_SRGB,
                .extent = { .width = 4U, .height = 4U, .depth = 1U },
                .mipLevels = 1U,
                .arrayLayers = 1U,
                .samples = g_MSAASamples,
                .tiling = g_ImageTiling,
                .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .initialLayout = g_UndefinedLayout
        };

        std::uint32_t MemoryType;
        CheckVulkanResult(vmaFindMemoryTypeIndexForImageInfo(g_Allocator, &ImageViewCreateInfo, &AllocationCreateInfo, &MemoryType));

        VmaPoolCreateInfo const PoolCreateInfo { .memoryTypeIndex = MemoryType, .flags = VMA_POOL_CREATE_LINEAR_ALGORITHM_BIT, .priority = 1.F };

        CheckVulkanResult(vmaCreatePool(g_Allocator, &PoolCreateInfo, &g_ImagePool));
        vmaSetPoolName(g_Allocator, g_ImagePool, "Image Pool");
    }
}

void RenderCore::ReleaseMemoryResources()
{
    g_UniformAllocation.DestroyResources(g_Allocator);
    g_StorageAllocation.DestroyResources(g_Allocator);

    for (auto &ImageIter : g_AllocatedImages | std::views::values)
    {
        ImageIter.DestroyResources(g_Allocator);
    }
    g_AllocatedImages.clear();

    vmaDestroyPool(g_Allocator, g_StagingUniformBufferPool);
    g_StagingUniformBufferPool = VK_NULL_HANDLE;

    vmaDestroyPool(g_Allocator, g_StagingStorageBufferPool);
    g_StagingStorageBufferPool = VK_NULL_HANDLE;

    vmaDestroyPool(g_Allocator, g_DescriptorBufferPool);
    g_DescriptorBufferPool = VK_NULL_HANDLE;

    vmaDestroyPool(g_Allocator, g_UniformPool);
    g_UniformPool = VK_NULL_HANDLE;

    vmaDestroyPool(g_Allocator, g_StoragePool);
    g_StoragePool = VK_NULL_HANDLE;

    vmaDestroyPool(g_Allocator, g_ImagePool);
    g_ImagePool = VK_NULL_HANDLE;

    vmaDestroyAllocator(g_Allocator);
    g_Allocator = VK_NULL_HANDLE;
}

VmaAllocationInfo RenderCore::CreateBuffer(VkDeviceSize const &        Size,
                                           VkBufferUsageFlags const    Usage,
                                           strzilla::string_view const Identifier,
                                           VkBuffer &                  Buffer,
                                           VmaAllocation &             Allocation)
{
    bool const IsStagingBuffer = Identifier.starts_with("STAGING_");
    bool const IsUniform = Usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bool const IsStorage = Usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    VmaAllocationCreateInfo AllocationCreateInfo {
            .flags = 0U,
            .usage = g_ModelMemoryUsage,
            .pool = IsStagingBuffer ? (IsUniform ? g_StagingUniformBufferPool : g_StagingStorageBufferPool) : (IsUniform ? g_UniformPool : g_StoragePool)
    };

    if (Usage & VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT)
    {
        AllocationCreateInfo.pool = g_DescriptorBufferPool;
        AllocationCreateInfo.flags |= g_MapMemoryFlag;
    }
    else if (IsStagingBuffer || IsUniform || IsStorage || Identifier == "IMGUI_RENDER")
    {
        AllocationCreateInfo.flags |= g_MapMemoryFlag;

        if (IsStagingBuffer)
        {
            AllocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
        }
    }

    VkBufferCreateInfo const BufferCreateInfo { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = Size, .usage = Usage };

    VmaAllocator const &Allocator = GetAllocator();

    VmaAllocationInfo MemoryAllocationInfo;
    CheckVulkanResult(vmaCreateBuffer(Allocator, &BufferCreateInfo, &AllocationCreateInfo, &Buffer, &Allocation, &MemoryAllocationInfo));

    vmaSetAllocationName(Allocator, Allocation, std::data(std::format("Buffer: {}", std::data(Identifier))));

    return MemoryAllocationInfo;
}

void RenderCore::CopyBuffer(VkCommandBuffer const &CommandBuffer, VkBuffer const &Source, VkBuffer const &Destination, VkDeviceSize const &Size)
{
    VkBufferCopy const BufferCopy { .size = Size, };
    vkCmdCopyBuffer(CommandBuffer, Source, Destination, 1U, &BufferCopy);
}

void RenderCore::CreateUniformBuffers(BufferAllocation &BufferAllocation, VkDeviceSize const BufferSize, strzilla::string_view const Identifier)
{
    VmaAllocator const &         Allocator  = GetAllocator();
    constexpr VkBufferUsageFlags UsageFlags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    BufferAllocation.Size = BufferSize;
    CreateBuffer(BufferSize, UsageFlags, Identifier, BufferAllocation.Buffer, BufferAllocation.Allocation);
    vmaMapMemory(Allocator, BufferAllocation.Allocation, &BufferAllocation.MappedData);
}

void RenderCore::CreateImage(VkFormat const &            ImageFormat,
                             VkExtent2D const &          Extent,
                             VkImageTiling const &       Tiling,
                             VkImageUsageFlags const     ImageUsage,
                             VmaMemoryUsage const        MemoryUsage,
                             strzilla::string_view const Identifier,
                             VkImage &                   Image,
                             VmaAllocation &             Allocation)
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
            .initialLayout = g_UndefinedLayout
    };

    VmaAllocationCreateInfo const ImageCreateInfo { .usage = MemoryUsage, .pool = g_ImagePool, .priority = 1.F };

    VmaAllocator const &Allocator = GetAllocator();

    VmaAllocationInfo AllocationInfo;
    CheckVulkanResult(vmaCreateImage(Allocator, &ImageViewCreateInfo, &ImageCreateInfo, &Image, &Allocation, &AllocationInfo));

    vmaSetAllocationName(Allocator, Allocation, std::data(std::format("Image: {}", std::data(Identifier))));
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

    VkDevice const &LogicalDevice = GetLogicalDevice();
    CheckVulkanResult(vkCreateImageView(LogicalDevice, &ImageViewCreateInfo, nullptr, &ImageView));
}

void RenderCore::CreateTextureImageView(ImageAllocation &Allocation, VkFormat const ImageFormat)
{
    CreateImageView(Allocation.Image, ImageFormat, g_ImageAspect, Allocation.View);
}

void RenderCore::CopyBufferToImage(VkCommandBuffer const &CommandBuffer, VkBuffer const &Source, VkImage const &Destination, VkExtent2D const &Extent)
{
    VkBufferImageCopy const BufferImageCopy {
            .bufferOffset = 0U,
            .bufferRowLength = 0U,
            .bufferImageHeight = 0U,
            .imageSubresource = { .aspectMask = g_ImageAspect, .mipLevel = 0U, .baseArrayLayer = 0U, .layerCount = 1U },
            .imageOffset = { .x = 0U, .y = 0U, .z = 0U },
            .imageExtent = { .width = Extent.width, .height = Extent.height, .depth = 1U }
    };

    vkCmdCopyBufferToImage(CommandBuffer, Source, Destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &BufferImageCopy);
}

std::tuple<std::uint32_t, VkBuffer, VmaAllocation> RenderCore::AllocateTexture(VkCommandBuffer const &CommandBuffer,
                                                                               unsigned char const   *Data,
                                                                               std::uint32_t const    Width,
                                                                               std::uint32_t const    Height,
                                                                               VkFormat const         ImageFormat,
                                                                               VkDeviceSize const     AllocationSize)
{
    if (std::empty(g_AllocatedImages))
    {
        g_ImageAllocationIDCounter.fetch_sub(g_ImageAllocationIDCounter.load());
    }

    std::pair<VkBuffer, VmaAllocation> Output;
    VmaAllocationInfo StagingInfo = CreateBuffer(AllocationSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, "STAGING_TEXTURE", Output.first, Output.second);

    VmaAllocator const &Allocator = GetAllocator();

    CheckVulkanResult(vmaMapMemory(Allocator, Output.second, &StagingInfo.pMappedData));
    std::memcpy(StagingInfo.pMappedData, Data, AllocationSize);

    ImageAllocation NewAllocation { .Extent = { .width = Width, .height = Height }, .Format = ImageFormat };

    CreateImage(ImageFormat,
                NewAllocation.Extent,
                g_ImageTiling,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                g_TextureMemoryUsage,
                "TEXTURE",
                NewAllocation.Image,
                NewAllocation.Allocation);

    constexpr auto TransferLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    RequestImageLayoutTransition<g_UndefinedLayout, TransferLayout, g_ImageAspect>(CommandBuffer,
                                                                                   NewAllocation.Image,
                                                                                   NewAllocation.Format);

    CopyBufferToImage(CommandBuffer, Output.first, NewAllocation.Image, NewAllocation.Extent);

    RequestImageLayoutTransition<TransferLayout, g_ReadLayout, g_ImageAspect>(CommandBuffer,
                                                                              NewAllocation.Image,
                                                                              NewAllocation.Format);

    CreateImageView(NewAllocation.Image, NewAllocation.Format, g_ImageAspect, NewAllocation.View);
    vmaUnmapMemory(Allocator, Output.second);

    std::uint32_t const BufferID = g_ImageAllocationIDCounter.fetch_add(1U);
    g_AllocatedImages.emplace(BufferID, std::move(NewAllocation));

    if (g_ImageAllocationCounter.contains(BufferID))
    {
        g_ImageAllocationCounter.at(BufferID) += 1U;
    }
    else
    {
        g_ImageAllocationCounter.emplace(BufferID, 1U);
    }

    return { BufferID, Output.first, Output.second };
}

void RenderCore::AllocateModelsBuffers(std::vector<std::shared_ptr<Object>> const &Objects)
{
    if (g_UniformAllocation.IsValid())
    {
        g_UniformAllocation.DestroyResources(g_Allocator);
    }

    if (g_StorageAllocation.IsValid())
    {
        g_StorageAllocation.DestroyResources(g_Allocator);
    }

    VmaAllocator const& Allocator = GetAllocator();

    std::vector<Meshlet>   Meshlets;
    std::vector<glm::uint> Indices;
    std::vector<Vertex>    Vertices;

    for (auto const &ObjectIter : Objects)
    {
        auto const &Mesh = ObjectIter->GetMesh();

        auto const &MeshMeshlets = Mesh->GetMeshlets();
        auto const &MeshIndices  = Mesh->GetIndices();
        auto const &MeshVertices = Mesh->GetVertices();

        Mesh->SetMeshletsOffset(std::size(Meshlets) * sizeof(Meshlet));
        Mesh->SetIndicesOffset (std::size(Indices)  * sizeof(glm::uint));
        Mesh->SetVerticesOffset(std::size(Vertices) * sizeof(Vertex));

        Meshlets.insert(std::end(Meshlets), std::begin(MeshMeshlets), std::end(MeshMeshlets));
        Indices .insert(std::end(Indices),  std::begin(MeshIndices),  std::end(MeshIndices));
        Vertices.insert(std::end(Vertices), std::begin(MeshVertices), std::end(MeshVertices));
    }

    VkDeviceSize const MeshletsBufferSize = sizeof(Meshlet)          * std::size(Meshlets);
    VkDeviceSize const IndicesBufferSize  = sizeof(glm::uint)        * std::size(Indices);
    VkDeviceSize const VerticesBufferSize = sizeof(Vertex)           * std::size(Vertices);
    VkDeviceSize const ModelDataSize      = sizeof(ModelUniformData) * std::size(Objects);
    VkDeviceSize const MaterialDataSize   = sizeof(MaterialData)     * std::size(Objects);

    VkDeviceSize const UniformBufferSize = ModelDataSize + MaterialDataSize;
    VkDeviceSize const StorageBufferSize = MeshletsBufferSize + IndicesBufferSize + VerticesBufferSize;

    CreateBuffer(UniformBufferSize, g_UniformBufferUsage, "SCENE_UNIFORM_UNIFIED_BUFFER", g_UniformAllocation.Buffer, g_UniformAllocation.Allocation);
    CreateBuffer(StorageBufferSize, g_StorageBufferUsage, "SCENE_STORAGE_UNIFIED_BUFFER", g_StorageAllocation.Buffer, g_StorageAllocation.Allocation);

    CheckVulkanResult(vmaMapMemory(Allocator, g_UniformAllocation.Allocation, &g_UniformAllocation.MappedData));
    CheckVulkanResult(vmaMapMemory(Allocator, g_StorageAllocation.Allocation, &g_StorageAllocation.MappedData));

    CheckVulkanResult(vmaFlushAllocation(Allocator, g_UniformAllocation.Allocation, 0U, UniformBufferSize));
    CheckVulkanResult(vmaFlushAllocation(Allocator, g_StorageAllocation.Allocation, 0U, StorageBufferSize));

    vmaUnmapMemory(Allocator, g_UniformAllocation.Allocation);
    vmaUnmapMemory(Allocator, g_StorageAllocation.Allocation);

    for (auto const &ObjectIter : Objects)
    {
        if (auto const& ObjectMesh = ObjectIter->GetMesh())
        {
            std::size_t const CurrentIndex = std::distance(std::data(Objects), &ObjectIter);

            ObjectMesh->SetIndicesOffset (ObjectMesh->GetIndicesOffset()  + MeshletsBufferSize);
            ObjectMesh->SetVerticesOffset(ObjectMesh->GetVerticesOffset() + IndicesBufferSize);

            ObjectMesh->SetModelOffset   (sizeof(ModelUniformData) * CurrentIndex);
            ObjectMesh->SetMaterialOffset(sizeof(MaterialData)     * CurrentIndex + ModelDataSize);

            ObjectMesh->MarkAsRenderDirty();
            ObjectMesh->SetupUniformDescriptor();
        }
    }
}

void RenderCore::SaveImageToFile(VkImage const &Image, strzilla::string_view const Path, VkExtent2D const &Extent)
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
        VkImageSubresource SubResource { .aspectMask = g_ImageAspect, .mipLevel = 0, .arrayLayer = 0 };

        VkDevice const &LogicalDevice = GetLogicalDevice();
        vkGetImageSubresourceLayout(LogicalDevice, Image, &SubResource, &Layout);

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
                .imageSubresource = { .aspectMask = g_ImageAspect, .mipLevel = 0U, .baseArrayLayer = 0U, .layerCount = 1U },
                .imageOffset = { 0U, 0U, 0U },
                .imageExtent = { .width = Extent.width, .height = Extent.height, .depth = 1U }
        };

        VkImageMemoryBarrier2 PreCopyBarrier {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
                .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                .oldLayout = g_UndefinedLayout,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = Image,
                .subresourceRange = { .aspectMask = g_ImageAspect, .baseMipLevel = 0U, .levelCount = 1U, .baseArrayLayer = 0U, .layerCount = 1U }
        };

        VkImageMemoryBarrier2 PostCopyBarrier {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
                .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_NONE_KHR,
                .dstAccessMask = VK_ACCESS_2_NONE_KHR,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .newLayout = g_PresentLayout,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = Image,
                .subresourceRange = { .aspectMask = g_ImageAspect, .baseMipLevel = 0U, .levelCount = 1U, .baseArrayLayer = 0U, .layerCount = 1U }
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

void TextureDeleter::operator()(Texture *const Texture) const
{
    std::uint32_t const BufferIndex = Texture->GetBufferIndex();
    g_ImageAllocationCounter.at(BufferIndex) -= 1U;

    if (g_ImageAllocationCounter.at(BufferIndex) == 0U)
    {
        g_AllocatedImages.at(BufferIndex).DestroyResources(g_Allocator);
        g_AllocatedImages.erase(BufferIndex);
        g_ImageAllocationCounter.erase(BufferIndex);
    }
}

void RenderCore::PrintMemoryAllocatorStats(bool const DetailedMap)
{
    char *Stats;
    vmaBuildStatsString(g_Allocator, &Stats, DetailedMap);
    BOOST_LOG_TRIVIAL(info) << Stats;
    vmaFreeStatsString(g_Allocator, Stats);
}

strzilla::string RenderCore::GetMemoryAllocatorStats(bool const DetailedMap)
{
    char *Stats = nullptr;
    vmaBuildStatsString(g_Allocator, &Stats, DetailedMap);
    strzilla::string Output { Stats };
    vmaFreeStatsString(g_Allocator, Stats);

    return Output;
}
