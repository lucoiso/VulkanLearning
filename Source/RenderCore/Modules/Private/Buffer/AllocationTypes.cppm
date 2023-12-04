// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <vk_mem_alloc.h>
#include <volk.h>

module RenderCore.Types.AllocationTypes;

import Timer.ExecutionCounter;

using namespace RenderCore;

bool ImageAllocation::IsValid() const
{
    return Image != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE;
}

void ImageAllocation::DestroyResources(VkDevice const& LogicalDevice, VmaAllocator const& Allocator)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    if (Image != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE)
    {
        vmaDestroyImage(Allocator, Image, Allocation);
        Image      = VK_NULL_HANDLE;
        Allocation = VK_NULL_HANDLE;
    }

    if (View != VK_NULL_HANDLE)
    {
        vkDestroyImageView(LogicalDevice, View, nullptr);
        View = VK_NULL_HANDLE;

        if (Image != VK_NULL_HANDLE)
        {
            Image = VK_NULL_HANDLE;
        }
    }

    if (Sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(LogicalDevice, Sampler, nullptr);
        Sampler = VK_NULL_HANDLE;
    }
}

bool BufferAllocation::IsValid() const
{
    return Buffer != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE;
}

void BufferAllocation::DestroyResources(VmaAllocator const& Allocator)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    if (Buffer != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE)
    {
        if (MappedData)
        {
            vmaUnmapMemory(Allocator, Allocation);
            MappedData = nullptr;
        }

        vmaDestroyBuffer(Allocator, Buffer, Allocation);
        Allocation = VK_NULL_HANDLE;
        Buffer     = VK_NULL_HANDLE;
    }
}

bool ObjectAllocation::IsValid() const
{
    return VertexBuffer.IsValid() && IndexBuffer.IsValid() && IndicesCount != 0U;
}

void ObjectAllocation::DestroyResources(VkDevice const& LogicalDevice, VmaAllocator const& Allocator)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    VertexBuffer.DestroyResources(Allocator);
    IndexBuffer.DestroyResources(Allocator);

    for (ImageAllocation& TextureImage: TextureImages)
    {
        TextureImage.DestroyResources(LogicalDevice, Allocator);
    }

    UniformBuffer.DestroyResources(Allocator);
    IndicesCount = 0U;
}