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
import RenderCore.Utils.Constants;

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

    constexpr VkImageAspectFlags AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    constexpr VkImageUsageFlags  UsageFlags  = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    std::ranges::for_each(g_ViewportImages,
                          [&](ImageAllocation &ImageIter)
                          {
                              ImageIter.Extent = SurfaceProperties.Extent;
                              ImageIter.Format = SurfaceProperties.Format.format;

                              CreateImage(SurfaceProperties.Format.format,
                                          SurfaceProperties.Extent,
                                          g_ImageTiling,
                                          UsageFlags,
                                          g_TextureMemoryUsage,
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
