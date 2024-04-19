// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <vma/vk_mem_alloc.h>
#include <Volk/volk.h>

module RenderCore.Types.Allocation;

using namespace RenderCore;

bool ImageAllocation::IsValid() const
{
    return Image != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE;
}

void ImageAllocation::DestroyResources(VmaAllocator const &Allocator)
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

void BufferAllocation::DestroyResources(VmaAllocator const &Allocator)
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

bool DescriptorData::IsValid() const
{
    return Buffer.IsValid() && SetLayout != VK_NULL_HANDLE;
}

void DescriptorData::DestroyResources(VmaAllocator const &Allocator)
{
    if (SetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(volkGetLoadedDevice(), SetLayout, nullptr);
        SetLayout = VK_NULL_HANDLE;
    }

    Buffer.DestroyResources(Allocator);
    LayoutOffset = 0U;
    LayoutSize   = 0U;
}

void DescriptorData::SetDescriptorLayoutSize(VkDeviceSize const &MinAlignment)
{
    vkGetDescriptorSetLayoutSizeEXT(volkGetLoadedDevice(), SetLayout, &LayoutSize);
    LayoutSize = LayoutSize + MinAlignment - 1 & ~(MinAlignment - 1);

    vkGetDescriptorSetLayoutBindingOffsetEXT(volkGetLoadedDevice(), SetLayout, 0U, &LayoutOffset);
}
