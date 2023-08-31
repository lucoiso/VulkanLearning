// Copyright Notice: [...]

#pragma once

#include "RenderCoreModule.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace RenderCore
{
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

        std::uint32_t DrawFrame(const VkSwapchainKHR& SwapChain);
        void RecordCommandBuffers(const VkRenderPass& RenderPass, const VkPipeline& Pipeline, const VkExtent2D& Extent, const std::vector<VkFramebuffer>& FrameBuffers, const std::vector<VkBuffer>& VertexBuffers, const std::vector<VkDeviceSize>& Offsets);
        void SubmitCommandBuffers(const VkQueue& GraphicsQueue);
        void PresentFrame(const VkQueue& PresentQueue, const VkSwapchainKHR& SwapChain, const std::uint32_t ImageIndex);

        bool IsInitialized() const;
        [[nodiscard]] const VkCommandPool& GetCommandPool() const;
        [[nodiscard]] const std::vector<VkCommandBuffer>& GetCommandBuffers() const;

    private:
        const VkDevice& m_Device;
        VkCommandPool m_CommandPool;
        std::vector<VkCommandBuffer> m_CommandBuffers;
        VkSemaphore m_ImageAvailableSemaphore;
        VkSemaphore m_RenderFinishedSemaphore;
        VkFence m_Fence;
    };
}
