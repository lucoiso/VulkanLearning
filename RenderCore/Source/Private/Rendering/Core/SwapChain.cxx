// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.Runtime.SwapChain;

import RenderCore.Renderer;
import RenderCore.Runtime.Device;
import RenderCore.Runtime.Instance;
import RenderCore.Runtime.Synchronization;
import RenderCore.Runtime.Memory;
import RenderCore.Utils.Helpers;

using namespace RenderCore;

void RenderCore::CreateSwapChain(SurfaceProperties const &SurfaceProperties, VkSurfaceCapabilitiesKHR const &SurfaceCapabilities)
{
    auto const QueueFamilyIndices      = GetUniqueQueueFamilyIndicesU32();
    auto const QueueFamilyIndicesCount = static_cast<std::uint32_t>(std::size(QueueFamilyIndices));

    g_OldSwapChain     = g_SwapChain;
    g_CachedProperties = SurfaceProperties;

    VkSwapchainCreateInfoKHR const SwapChainCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = GetSurface(),
            .minImageCount = g_ImageCount,
            .imageFormat = g_CachedProperties.Format.format,
            .imageColorSpace = g_CachedProperties.Format.colorSpace,
            .imageExtent = g_CachedProperties.Extent,
            .imageArrayLayers = 1U,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = QueueFamilyIndicesCount > 1U ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = QueueFamilyIndicesCount,
            .pQueueFamilyIndices = std::data(QueueFamilyIndices),
            .preTransform = SurfaceCapabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = g_CachedProperties.Mode,
            .clipped = true,
            .oldSwapchain = g_OldSwapChain
    };

    VkDevice const &LogicalDevice = GetLogicalDevice();

    CheckVulkanResult(vkCreateSwapchainKHR(LogicalDevice, &SwapChainCreateInfo, nullptr, &g_SwapChain));

    if (g_OldSwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(LogicalDevice, g_OldSwapChain, nullptr);
        g_OldSwapChain = VK_NULL_HANDLE;
    }

    std::uint32_t ImageCount = 0U;
    CheckVulkanResult(vkGetSwapchainImagesKHR(LogicalDevice, g_SwapChain, &ImageCount, nullptr));

    std::vector<VkImage> SwapChainImages(ImageCount, VK_NULL_HANDLE);
    CheckVulkanResult(vkGetSwapchainImagesKHR(LogicalDevice, g_SwapChain, &ImageCount, std::data(SwapChainImages)));

    std::ranges::transform(SwapChainImages,
                           std::begin(g_SwapChainImages),
                           [SurfaceProperties](VkImage const &Image)
                           {
                               return ImageAllocation {
                                       .Image = Image,
                                       .Extent = SurfaceProperties.Extent,
                                       .Format = SurfaceProperties.Format.format
                               };
                           });

    CreateSwapChainImageViews(g_SwapChainImages);
}

bool RenderCore::RequestSwapChainImage(std::uint32_t &Output)
{
    VkDevice const &   LogicalDevice = GetLogicalDevice();
    std::uint8_t const SyncIndex     = Output + 1U >= g_ImageCount ? 0U : Output + 1U;
    VkSemaphore const &Semaphore     = GetImageAvailableSemaphore(SyncIndex);

    if (!Renderer::GetUseDefaultSync())
    {
        WaitAndResetFence(SyncIndex);
    }

    return vkAcquireNextImageKHR(LogicalDevice, g_SwapChain, g_Timeout, Semaphore, VK_NULL_HANDLE, &Output) == VK_SUCCESS;
}

void RenderCore::CreateSwapChainImageViews(std::array<ImageAllocation, g_ImageCount> &Images)
{
    std::for_each(std::execution::unseq,
                  std::begin(Images),
                  std::end(Images),
                  [&](ImageAllocation &ImageIter)
                  {
                      CreateImageView(ImageIter.Image, ImageIter.Format, g_ImageAspect, ImageIter.View);
                  });
}

void RenderCore::PresentFrame(std::uint32_t const ImageIndice)
{
    VkPresentInfoKHR const PresentInfo {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1U,
            .pWaitSemaphores = &GetRenderFinishedSemaphore(ImageIndice),
            .swapchainCount = 1U,
            .pSwapchains = &g_SwapChain,
            .pImageIndices = &ImageIndice
    };

    if (VkResult const PresentResult = vkQueuePresentKHR(GetGraphicsQueue().second, &PresentInfo);
        PresentResult != VK_SUBOPTIMAL_KHR && PresentResult != VK_ERROR_OUT_OF_DATE_KHR)
    {
        CheckVulkanResult(PresentResult);
    }
}

void RenderCore::ReleaseSwapChainResources()
{
    VkDevice const &LogicalDevice = GetLogicalDevice();
    DestroySwapChainImages();

    if (g_SwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(LogicalDevice, g_SwapChain, nullptr);
        g_SwapChain = VK_NULL_HANDLE;
    }

    if (g_OldSwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(LogicalDevice, g_OldSwapChain, nullptr);
        g_OldSwapChain = VK_NULL_HANDLE;
    }

    vkDestroySurfaceKHR(GetInstance(), g_Surface, nullptr);
    g_Surface = VK_NULL_HANDLE;
}

void RenderCore::DestroySwapChainImages()
{
    VmaAllocator const &Allocator = GetAllocator();
    std::for_each(std::execution::unseq,
                  std::begin(g_SwapChainImages),
                  std::end(g_SwapChainImages),
                  [&](ImageAllocation &ImageIter)
                  {
                      ImageIter.DestroyResources(Allocator);
                  });
}
