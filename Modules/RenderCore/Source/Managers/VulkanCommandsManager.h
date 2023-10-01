// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

#pragma once

#include <limits>
#include <optional>
#include <volk.h>

namespace RenderCore
{
    constexpr std::uint32_t g_Timeout = std::numeric_limits<std::uint32_t>::max();

    class VulkanCommandsManager final
    {
    public:
        VulkanCommandsManager();

        VulkanCommandsManager(VulkanCommandsManager const&)            = delete;
        VulkanCommandsManager& operator=(VulkanCommandsManager const&) = delete;

        ~VulkanCommandsManager();

        static VulkanCommandsManager& Get();

        void Shutdown();

        [[nodiscard]] static VkCommandPool CreateCommandPool(std::uint8_t FamilyQueueIndex);

        void CreateSynchronizationObjects();
        void DestroySynchronizationObjects();

        [[nodiscard]] std::optional<std::int32_t> DrawFrame() const;

        void RecordCommandBuffers(std::uint32_t ImageIndex);
        void SubmitCommandBuffers();
        void PresentFrame(std::uint32_t ImageIndice);

    private:
        void CreateGraphicsCommandPool();
        void AllocateCommandBuffer();
        void WaitAndResetFences() const;

        VkCommandPool m_CommandPool;
        VkCommandBuffer m_CommandBuffer;
        VkSemaphore m_ImageAvailableSemaphore;
        VkSemaphore m_RenderFinishedSemaphore;
        VkFence m_Fence;
        bool m_SynchronizationObjectsCreated;

        std::uint8_t m_FrameIndex;
    };
}// namespace RenderCore
