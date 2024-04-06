// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <algorithm>
#include <vma/vk_mem_alloc.h>

module RenderCore.Integrations.Viewport;

import RenderCore.Types.Allocation;
import RenderCore.Types.SurfaceProperties;
import RenderCore.Runtime.Memory;
import RenderCore.Runtime.SwapChain;

using namespace RenderCore;

#ifdef VULKAN_RENDERER_ENABLE_IMGUI

std::vector<ImageAllocation> g_ViewportImages {};

void RenderCore::CreateViewportResources(SurfaceProperties const &SurfaceProperties)
{
    std::ranges::for_each(g_ViewportImages,
                          [&](ImageAllocation &ImageIter)
                          {
                              ImageIter.DestroyResources(GetAllocator());
                          });
    g_ViewportImages.resize(std::size(GetSwapChainImages()));

    constexpr VmaMemoryUsage MemoryUsage = VMA_MEMORY_USAGE_AUTO;
    constexpr VkImageAspectFlags AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    constexpr VkImageTiling Tiling = VK_IMAGE_TILING_LINEAR;
    constexpr VkImageUsageFlags UsageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                             VK_IMAGE_USAGE_SAMPLED_BIT;
    constexpr VmaAllocationCreateFlags MemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

    std::ranges::for_each(g_ViewportImages,
                          [&](ImageAllocation &ImageIter)
                          {
                              CreateImage(SurfaceProperties.Format.format,
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

std::vector<ImageAllocation> const &RenderCore::GetViewportImages()
{
    return g_ViewportImages;
}

void RenderCore::DestroyViewportImages()
{
    std::ranges::for_each(g_ViewportImages,
                          [&](ImageAllocation &ImageIter)
                          {
                              ImageIter.DestroyResources(GetAllocator());
                          });
    g_ViewportImages.clear();
}

#endif
