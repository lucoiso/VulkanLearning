// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <vma/vk_mem_alloc.h>
#include <Volk/volk.h>
#include <algorithm>

module RenderCore.Types.AllocationTypes;

using namespace RenderCore;

bool ImageAllocation::IsValid() const
{
    return Image != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE;
}

void ImageAllocation::DestroyResources(VmaAllocator const& Allocator)
{
    if (Image != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE)
    {
        vmaDestroyImage(Allocator, Image, Allocation);
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
}

bool BufferAllocation::IsValid() const
{
    return Buffer != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE;
}

void BufferAllocation::DestroyResources(VmaAllocator const& Allocator)
{
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

void ObjectAllocation::DestroyResources(VmaAllocator const& Allocator)
{
    std::ranges::for_each(TextureImages,
                          [&](ImageAllocation& TextureImageIter)
                          {
                              TextureImageIter.DestroyResources(Allocator);
                          });

    VertexBuffer.DestroyResources(Allocator);
    IndexBuffer.DestroyResources(Allocator);
    UniformBuffer.DestroyResources(Allocator);
    IndicesCount = 0U;
}
