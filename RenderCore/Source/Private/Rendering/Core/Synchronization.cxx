// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.Runtime.Synchronization;

import RenderCore.Renderer;
import RenderCore.Runtime.Device;
import RenderCore.Runtime.Command;
import RenderCore.Utils.Helpers;

using namespace RenderCore;

void RenderCore::WaitAndResetFence(std::uint32_t const Index)
{
    if (!Renderer::GetUseDefaultSync() && g_Fences.at(Index) == VK_NULL_HANDLE || !g_FenceInUse.at(Index))
    {
        return;
    }

    VkDevice const &LogicalDevice = GetLogicalDevice();
    CheckVulkanResult(vkWaitForFences(LogicalDevice, 1U, &g_Fences.at(Index), false, g_Timeout));
    CheckVulkanResult(vkResetFences(LogicalDevice, 1U, &g_Fences.at(Index)));
    g_FenceInUse.at(Index) = false;

    ResetCommandPool(Index);
}

void RenderCore::CreateSynchronizationObjects()
{
    VkDevice const &LogicalDevice = GetLogicalDevice();

    constexpr VkSemaphoreCreateInfo SemaphoreCreateInfo { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    for (auto &Semaphore : g_ImageAvailableSemaphores)
    {
        CheckVulkanResult(vkCreateSemaphore(LogicalDevice, &SemaphoreCreateInfo, nullptr, &Semaphore));
    }

    for (auto &Semaphore : g_RenderFinishedSemaphores)
    {
        CheckVulkanResult(vkCreateSemaphore(LogicalDevice, &SemaphoreCreateInfo, nullptr, &Semaphore));
    }

    constexpr VkFenceCreateInfo FenceCreateInfo { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };
    for (auto &Fence : g_Fences)
    {
        CheckVulkanResult(vkCreateFence(LogicalDevice, &FenceCreateInfo, nullptr, &Fence));
    }

    CheckVulkanResult(vkResetFences(LogicalDevice, static_cast<std::uint32_t>(std::size(g_Fences)), data(g_Fences)));
}

void RenderCore::ReleaseSynchronizationObjects()
{
    VkDevice const &LogicalDevice = GetLogicalDevice();
    vkDeviceWaitIdle(LogicalDevice);

    for (auto &Semaphore : g_ImageAvailableSemaphores)
    {
        if (Semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(LogicalDevice, Semaphore, nullptr);
            Semaphore = VK_NULL_HANDLE;
        }
    }

    for (auto &Semaphore : g_RenderFinishedSemaphores)
    {
        if (Semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(LogicalDevice, Semaphore, nullptr);
            Semaphore = VK_NULL_HANDLE;
        }
    }

    for (auto &Fence : g_Fences)
    {
        if (Fence != VK_NULL_HANDLE)
        {
            vkDestroyFence(LogicalDevice, Fence, nullptr);
            Fence = VK_NULL_HANDLE;
        }
    }

    ResetFenceStatus();
}

void RenderCore::ResetSemaphores()
{
    vkQueueWaitIdle(GetGraphicsQueue().second);

    VkDevice const &                LogicalDevice = GetLogicalDevice();
    constexpr VkSemaphoreCreateInfo SemaphoreCreateInfo { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

    for (auto &Semaphore : g_ImageAvailableSemaphores)
    {
        if (Semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(LogicalDevice, Semaphore, nullptr);
            CheckVulkanResult(vkCreateSemaphore(LogicalDevice, &SemaphoreCreateInfo, nullptr, &Semaphore));
        }
    }

    for (auto &Semaphore : g_RenderFinishedSemaphores)
    {
        if (Semaphore != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(LogicalDevice, Semaphore, nullptr);
            CheckVulkanResult(vkCreateSemaphore(LogicalDevice, &SemaphoreCreateInfo, nullptr, &Semaphore));
        }
    }
}

void RenderCore::ResetFenceStatus()
{
    for (std::uint8_t Iterator = 0U; Iterator < g_ImageCount; ++Iterator)
    {
        if (bool &FenceStatus = g_FenceInUse.at(Iterator);
            FenceStatus)
        {
            WaitAndResetFence(Iterator);
            FenceStatus = false;
        }
    }
}
