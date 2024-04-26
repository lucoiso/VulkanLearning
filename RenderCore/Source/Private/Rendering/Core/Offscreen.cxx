// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <algorithm>
#include <execution>
#include <vma/vk_mem_alloc.h>

module RenderCore.Integrations.Offscreen;

import RenderCore.Types.Allocation;
import RenderCore.Types.SurfaceProperties;
import RenderCore.Runtime.Memory;
import RenderCore.Runtime.SwapChain;
import RenderCore.Utils.Constants;

using namespace RenderCore;

std::vector<ImageAllocation> g_OffscreenImages {};

void RenderCore::CreateOffscreenResources(SurfaceProperties const &SurfaceProperties)
{
    VmaAllocator const &Allocator = GetAllocator();

    std::for_each(std::execution::unseq,
                  std::begin(g_OffscreenImages),
                  std::end(g_OffscreenImages),
                  [&](ImageAllocation &ImageIter)
                  {
                      ImageIter.DestroyResources(Allocator);
                  });
    g_OffscreenImages.resize(std::size(GetSwapChainImages()));

    constexpr VkImageAspectFlags AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    constexpr VkImageUsageFlags  UsageFlags  = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    std::for_each(std::execution::unseq,
                  std::begin(g_OffscreenImages),
                  std::end(g_OffscreenImages),
                  [&](ImageAllocation &ImageIter)
                  {
                      ImageIter.Extent = SurfaceProperties.Extent;
                      ImageIter.Format = SurfaceProperties.Format.format;

                      CreateImage(SurfaceProperties.Format.format,
                                  SurfaceProperties.Extent,
                                  g_ImageTiling,
                                  UsageFlags,
                                  g_TextureMemoryUsage,
                                  "OFFSCREEN_IMAGE",
                                  ImageIter.Image,
                                  ImageIter.Allocation);

                      CreateImageView(ImageIter.Image, SurfaceProperties.Format.format, AspectFlags, ImageIter.View);
                  });
}

std::vector<ImageAllocation> const &RenderCore::GetOffscreenImages()
{
    return g_OffscreenImages;
}

void RenderCore::DestroyOffscreenImages()
{
    VmaAllocator const &Allocator = GetAllocator();

    std::for_each(std::execution::unseq,
                  std::begin(g_OffscreenImages),
                  std::end(g_OffscreenImages),
                  [&](ImageAllocation &ImageIter)
                  {
                      ImageIter.DestroyResources(Allocator);
                  });
    g_OffscreenImages.clear();
}
