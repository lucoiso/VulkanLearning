// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "RenderCoreModule.h"
#include "Types/VulkanUniformBufferObject.h"
#include <vulkan/vulkan_core.h>
#include <vector>
#include <unordered_map>
#include <optional>

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

        void SetGraphicsProcessingFamilyQueueIndex(const std::uint32_t FamilyQueueIndex);
        [[nodiscard]] VkCommandPool CreateCommandPool(const std::uint32_t FamilyQueueIndex);
        void CreateSynchronizationObjects();
        void DestroySynchronizationObjects();

        std::int32_t DrawFrame(const VkSwapchainKHR &SwapChains);
        
        struct BufferRecordParameters
        {
            const VkRenderPass RenderPass;
            const VkPipeline Pipeline;
            const VkExtent2D Extent;
            const std::vector<VkFramebuffer> FrameBuffers;
            const std::vector<VkBuffer> VertexBuffers;
            const std::vector<VkBuffer> IndexBuffers;
            const VkPipelineLayout PipelineLayout;
            const std::vector<VkDescriptorSet> DescriptorSets;
            const std::uint32_t IndexCount;
            const std::uint32_t ImageIndex;
            const std::vector<VkDeviceSize> Offsets;
        };

        void RecordCommandBuffers(const BufferRecordParameters &Parameters);
        void SubmitCommandBuffers(const VkQueue &Queue);
        void PresentFrame(const VkQueue &Queue, const VkSwapchainKHR &SwapChain, const std::uint32_t ImageIndice);

        std::uint32_t GetCurrentFrameIndex() const;

        bool IsInitialized() const;

    private:
        void CreateGraphicsCommandPool();
        void AllocateCommandBuffer();
        void WaitAndResetFences();

        const VkDevice &m_Device;
        VkCommandPool m_CommandPool;
        VkCommandBuffer m_CommandBuffer;
        VkSemaphore m_ImageAvailableSemaphore;
        VkSemaphore m_RenderFinishedSemaphore;
        VkFence m_Fence;
        std::uint32_t m_CurrentFrameIndex;
        bool m_SynchronizationObjectsCreated;
        std::optional<std::uint32_t> m_GraphicsProcessingFamilyQueueIndex;
    };
}
