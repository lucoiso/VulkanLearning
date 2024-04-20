// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;
#include <algorithm>
#include <vector>
#include <execution>
#include <Volk/volk.h>
#include <vma/vk_mem_alloc.h>

#ifdef GLFW_INCLUDE_VULKAN
    #undef GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

module RenderCore.Runtime.SwapChain;

import RenderCore.Runtime.Device;
import RenderCore.Runtime.Synchronization;
import RenderCore.Runtime.Memory;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;

using namespace RenderCore;

VkSurfaceKHR                 g_Surface { VK_NULL_HANDLE };
VkSwapchainKHR               g_SwapChain { VK_NULL_HANDLE };
VkSwapchainKHR               g_OldSwapChain { VK_NULL_HANDLE };
std::vector<ImageAllocation> g_SwapChainImages {};

void RenderCore::CreateVulkanSurface(GLFWwindow *const Window)
{
    CheckVulkanResult(glfwCreateWindowSurface(volkGetLoadedInstance(), Window, nullptr, &g_Surface));
}

void RenderCore::CreateSwapChain(SurfaceProperties const &SurfaceProperties, VkSurfaceCapabilitiesKHR const &SurfaceCapabilities)
{
    auto const QueueFamilyIndices      = GetUniqueQueueFamilyIndicesU32();
    auto const QueueFamilyIndicesCount = static_cast<std::uint32_t>(std::size(QueueFamilyIndices));

    g_OldSwapChain = g_SwapChain;

    VkSwapchainCreateInfoKHR const SwapChainCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = GetSurface(),
            .minImageCount = g_MinImageCount,
            .imageFormat = SurfaceProperties.Format.format,
            .imageColorSpace = SurfaceProperties.Format.colorSpace,
            .imageExtent = SurfaceProperties.Extent,
            .imageArrayLayers = 1U,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = QueueFamilyIndicesCount > 1U ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = QueueFamilyIndicesCount,
            .pQueueFamilyIndices = std::data(QueueFamilyIndices),
            .preTransform = SurfaceCapabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = SurfaceProperties.Mode,
            .clipped = VK_TRUE,
            .oldSwapchain = g_OldSwapChain
    };

    VkDevice const &LogicalDevice = GetLogicalDevice();

    CheckVulkanResult(vkCreateSwapchainKHR(LogicalDevice, &SwapChainCreateInfo, nullptr, &g_SwapChain));

    if (g_OldSwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(LogicalDevice, g_OldSwapChain, nullptr);
        g_OldSwapChain = VK_NULL_HANDLE;
    }

    std::uint32_t Count = 0U;
    CheckVulkanResult(vkGetSwapchainImagesKHR(LogicalDevice, g_SwapChain, &Count, nullptr));

    std::vector<VkImage> SwapChainImages(Count, VK_NULL_HANDLE);
    CheckVulkanResult(vkGetSwapchainImagesKHR(LogicalDevice, g_SwapChain, &Count, std::data(SwapChainImages)));

    g_SwapChainImages.resize(Count, ImageAllocation());
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

std::optional<std::int32_t> RenderCore::RequestSwapChainImage()
{
    VkDevice const &LogicalDevice = GetLogicalDevice();
    std::uint32_t   Output        = 0U;

    if (vkAcquireNextImageKHR(LogicalDevice, g_SwapChain, g_Timeout, GetImageAvailableSemaphore(), VK_NULL_HANDLE, &Output) != VK_SUCCESS)
    {
        return std::nullopt;
    }

    return std::optional { static_cast<std::int32_t>(Output) };
}

void RenderCore::CreateSwapChainImageViews(std::vector<ImageAllocation> &Images)
{
    std::for_each(std::execution::unseq,
                  std::begin(Images),
                  std::end(Images),
                  [&](ImageAllocation &ImageIter)
                  {
                      CreateImageView(ImageIter.Image, ImageIter.Format, VK_IMAGE_ASPECT_COLOR_BIT, ImageIter.View);
                  });
}

void RenderCore::PresentFrame(std::uint32_t const ImageIndice)
{
    VkPresentInfoKHR const PresentInfo {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1U,
            .pWaitSemaphores = &GetRenderFinishedSemaphore(),
            .swapchainCount = 1U,
            .pSwapchains = &g_SwapChain,
            .pImageIndices = &ImageIndice
    };

    CheckVulkanResult(vkQueuePresentKHR(GetPresentationQueue().second, &PresentInfo));
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

    vkDestroySurfaceKHR(volkGetLoadedInstance(), g_Surface, nullptr);
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
    g_SwapChainImages.clear();
}

VkSurfaceKHR const &RenderCore::GetSurface()
{
    return g_Surface;
}

VkSwapchainKHR const &RenderCore::GetSwapChain()
{
    return g_SwapChain;
}

VkExtent2D const &RenderCore::GetSwapChainExtent()
{
    return g_SwapChainImages.at(0U).Extent;
}

VkFormat const &RenderCore::GetSwapChainImageFormat()
{
    return g_SwapChainImages.at(0U).Format;
}

std::vector<ImageAllocation> const &RenderCore::GetSwapChainImages()
{
    return g_SwapChainImages;
}
