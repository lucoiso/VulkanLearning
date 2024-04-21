// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <vector>
#include <Volk/volk.h>

module RenderCore.Runtime.Synchronization;

import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import RenderCore.Runtime.Device;

using namespace RenderCore;

VkSemaphore          g_ImageAvailableSemaphore {};
VkSemaphore          g_RenderFinishedSemaphore {};
std::vector<VkFence> g_Fences { g_MinImageCount, VK_NULL_HANDLE };

void RenderCore::WaitAndResetFence(std::uint32_t const Index)
{
    if (g_Fences.at(Index) == VK_NULL_HANDLE)
    {
        return;
    }

    VkDevice const &LogicalDevice = GetLogicalDevice();
    CheckVulkanResult(vkWaitForFences(LogicalDevice, 1U, &g_Fences.at(Index), VK_TRUE, g_Timeout));
    CheckVulkanResult(vkResetFences(LogicalDevice, 1U, &g_Fences.at(Index)));
}

void RenderCore::CreateSynchronizationObjects()
{
    VkDevice const &LogicalDevice = GetLogicalDevice();

    constexpr VkSemaphoreCreateInfo SemaphoreCreateInfo { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    CheckVulkanResult(vkCreateSemaphore(LogicalDevice, &SemaphoreCreateInfo, nullptr, &g_ImageAvailableSemaphore));
    CheckVulkanResult(vkCreateSemaphore(LogicalDevice, &SemaphoreCreateInfo, nullptr, &g_RenderFinishedSemaphore));

    constexpr VkFenceCreateInfo FenceCreateInfo { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };
    for (auto &Fence : g_Fences)
    {
        CheckVulkanResult(vkCreateFence(LogicalDevice, &FenceCreateInfo, nullptr, &Fence));
    }

    CheckVulkanResult(vkResetFences(LogicalDevice, static_cast<std::uint32_t>(size(g_Fences)), data(g_Fences)));
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

    for (auto &Fence : g_Fences)
    {
        if (Fence != VK_NULL_HANDLE)
        {
            vkDestroyFence(LogicalDevice, Fence, nullptr);
            Fence = VK_NULL_HANDLE;
        }
    }
}

void RenderCore::ResetSemaphores()
{
    vkQueueWaitIdle(GetGraphicsQueue().second);

    VkDevice const &                LogicalDevice = GetLogicalDevice();
    constexpr VkSemaphoreCreateInfo SemaphoreCreateInfo { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    if (g_ImageAvailableSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(LogicalDevice, g_ImageAvailableSemaphore, nullptr);
        CheckVulkanResult(vkCreateSemaphore(LogicalDevice, &SemaphoreCreateInfo, nullptr, &g_ImageAvailableSemaphore));
    }

    if (g_RenderFinishedSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(LogicalDevice, g_RenderFinishedSemaphore, nullptr);
        CheckVulkanResult(vkCreateSemaphore(LogicalDevice, &SemaphoreCreateInfo, nullptr, &g_RenderFinishedSemaphore));
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

VkFence const &RenderCore::GetFence(std::uint32_t const Index)
{
    return g_Fences.at(Index);
}
