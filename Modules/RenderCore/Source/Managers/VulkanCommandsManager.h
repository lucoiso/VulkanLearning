// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "RenderCoreModule.h"
#include "Types/VulkanUniformBufferObject.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>

namespace RenderCore
{
    constexpr std::uint32_t Timeout = std::numeric_limits<std::uint32_t>::max();

    class VulkanCommandsManager
    {
    public:
        VulkanCommandsManager() = delete;
        VulkanCommandsManager(const VulkanCommandsManager &) = delete;
        VulkanCommandsManager &operator=(const VulkanCommandsManager &) = delete;

        VulkanCommandsManager(const VkDevice &Device);
        ~VulkanCommandsManager();

        void Shutdown(const std::vector<VkQueue> &PendingQueues);

        void CreateCommandPool(const std::uint32_t FamilyQueueIndex, const bool bIsGraphicsIndex);
        void CreateCommandBuffers();
        void CreateSynchronizationObjects();
        void DestroySynchronizationObjects();

        std::unordered_map<VkSwapchainKHR, std::uint32_t> DrawFrame(const std::vector<VkSwapchainKHR> &SwapChains);
        
        struct BufferRecordParameters
        {
            const VkRenderPass &RenderPass;
            const VkPipeline &Pipeline;
            const VkExtent2D Extent;
            const std::vector<VkFramebuffer> &FrameBuffers;
            const std::vector<VkBuffer> &VertexBuffers;
            const std::vector<VkBuffer> &IndexBuffers;
            const VkPipelineLayout &PipelineLayout;
            const std::vector<VkDescriptorSet> &DescriptorSets;
            const std::uint32_t IndexCount;
            const std::uint32_t ImageIndex;
            const std::vector<VkDeviceSize> Offsets;
        };

        void RecordCommandBuffers(const BufferRecordParameters &Parameters);
        void SubmitCommandBuffers(const VkQueue &GraphicsQueue);
        void PresentFrame(const VkQueue &PresentQueue, const std::unordered_map<VkSwapchainKHR, std::uint32_t> &ImageIndicesData);

        std::uint32_t GetCurrentFrameIndex() const;

        bool IsInitialized() const;
        [[nodiscard]] const VkCommandPool &GetCommandPool(const std::uint32_t FamilyIndex) const;
        [[nodiscard]] const std::vector<VkCommandBuffer> &GetCommandBuffers() const;

    private:
        void WaitAndResetFences(const bool bCurrentFrame = false);

        const VkDevice &m_Device;
        std::unordered_map<std::uint32_t, VkCommandPool> m_CommandPools;
        std::vector<VkCommandBuffer> m_CommandBuffers;
        std::vector<VkSemaphore> m_ImageAvailableSemaphores;
        std::vector<VkSemaphore> m_RenderFinishedSemaphores;
        std::vector<VkFence> m_Fences;
        std::uint32_t m_CurrentFrameIndex;
        std::uint32_t m_GraphicsProcessingFamilyQueueIndex;
        bool m_SynchronizationObjectsCreated;
    };
}
