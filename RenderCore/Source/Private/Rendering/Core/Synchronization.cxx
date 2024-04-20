// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <Volk/volk.h>

module RenderCore.Runtime.Synchronization;

import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import RenderCore.Runtime.Device;

using namespace RenderCore;

VkSemaphore g_ImageAvailableSemaphore {};
VkSemaphore g_RenderFinishedSemaphore {};
VkFence     g_Fence {};

void RenderCore::WaitAndResetFences()
{
    if (g_Fence == VK_NULL_HANDLE)
    {
        return;
    }

    VkDevice const &LogicalDevice = GetLogicalDevice();
    CheckVulkanResult(vkWaitForFences(LogicalDevice, 1U, &g_Fence, VK_TRUE, g_Timeout));
    CheckVulkanResult(vkResetFences(LogicalDevice, 1U, &g_Fence));
}

void RenderCore::CreateSynchronizationObjects()
{
    constexpr VkSemaphoreCreateInfo SemaphoreCreateInfo { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    constexpr VkFenceCreateInfo FenceCreateInfo { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };

    VkDevice const &LogicalDevice = GetLogicalDevice();
    CheckVulkanResult(vkCreateSemaphore(LogicalDevice, &SemaphoreCreateInfo, nullptr, &g_ImageAvailableSemaphore));
    CheckVulkanResult(vkCreateSemaphore(LogicalDevice, &SemaphoreCreateInfo, nullptr, &g_RenderFinishedSemaphore));

    CheckVulkanResult(vkCreateFence(LogicalDevice, &FenceCreateInfo, nullptr, &g_Fence));
    CheckVulkanResult(vkResetFences(LogicalDevice, 1U, &g_Fence));
}

void RenderCore::ReleaseSynchronizationObjects()
{
    VkDevice const &LogicalDevice = GetLogicalDevice();
    vkDeviceWaitIdle(LogicalDevice);

    if (g_ImageAvailableSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(LogicalDevice, g_ImageAvailableSemaphore, nullptr);
        g_ImageAvailableSemaphore = VK_NULL_HANDLE;
    }

    if (g_RenderFinishedSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(LogicalDevice, g_RenderFinishedSemaphore, nullptr);
        g_RenderFinishedSemaphore = VK_NULL_HANDLE;
    }

    if (g_Fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(LogicalDevice, g_Fence, nullptr);
        g_Fence = VK_NULL_HANDLE;
    }
}

VkSemaphore const &RenderCore::GetImageAvailableSemaphore()
{
    return g_ImageAvailableSemaphore;
}

VkSemaphore const &RenderCore::GetRenderFinishedSemaphore()
{
    return g_RenderFinishedSemaphore;
}

VkFence const &RenderCore::GetFence()
{
    return g_Fence;
}
