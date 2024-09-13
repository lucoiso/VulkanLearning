// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.Runtime.Offscreen;

import RenderCore.Runtime.Memory;

using namespace RenderCore;

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

    constexpr VkImageUsageFlags UsageFlags = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

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

                      CreateImageView(ImageIter.Image, SurfaceProperties.Format.format, g_ImageAspect, ImageIter.View);
                  });
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
}
