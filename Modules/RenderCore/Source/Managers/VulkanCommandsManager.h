// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "RenderCoreModule.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace RenderCore
{
    constexpr std::uint32_t Timeout = std::numeric_limits<std::uint32_t>::max();

    class VulkanCommandsManager
    {
    public:
        VulkanCommandsManager() = delete;
        VulkanCommandsManager(const VulkanCommandsManager&) = delete;
        VulkanCommandsManager& operator=(const VulkanCommandsManager&) = delete;

        VulkanCommandsManager(const VkDevice& Device);
        ~VulkanCommandsManager();

        void Shutdown(const std::vector<VkQueue>& PendingQueues);

        void CreateCommandPool(const std::vector<VkFramebuffer>& FrameBuffers, const std::uint32_t GraphicsFamilyQueueIndex);
        void CreateSynchronizationObjects();
        void DestroySynchronizationObjects();

        std::vector<std::uint32_t> DrawFrame(const std::vector<VkSwapchainKHR>& SwapChains);
        void RecordCommandBuffers(const VkRenderPass& RenderPass, const VkPipeline& Pipeline, const std::vector<VkViewport>& Viewports, const std::vector<VkRect2D>& Scissors, const VkExtent2D& Extent, const std::vector<VkFramebuffer>& FrameBuffers, const std::vector<VkBuffer>& VertexBuffers, const std::vector<VkDeviceSize>& Offsets);
        void SubmitCommandBuffers(const VkQueue& GraphicsQueue);
        void PresentFrame(const VkQueue& PresentQueue, const std::vector<VkSwapchainKHR>& SwapChains, const std::vector<std::uint32_t>& ImageIndexes);

        bool IsInitialized() const;
        [[nodiscard]] const VkCommandPool& GetCommandPool() const;
        [[nodiscard]] const std::vector<VkCommandBuffer>& GetCommandBuffers() const;

    private:
        void WaitAndResetFences();

        const VkDevice& m_Device;
        VkCommandPool m_CommandPool;
        std::vector<VkCommandBuffer> m_CommandBuffers;
        std::vector<VkSemaphore> m_ImageAvailableSemaphores;
        std::vector<VkSemaphore> m_RenderFinishedSemaphores;
        std::vector<VkFence> m_Fences;

        std::uint32_t m_ProcessingUnitsCount;
    };
}
