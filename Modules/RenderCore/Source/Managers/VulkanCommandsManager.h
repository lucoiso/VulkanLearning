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

        void Shutdown();

        void CreateCommandPool(const std::vector<VkFramebuffer>& FrameBuffers, const std::uint32_t GraphicsFamilyQueueIndex);

        bool IsInitialized() const;
        [[nodiscard]] const VkCommandPool& GetCommandPool() const;
        [[nodiscard]] const std::vector<VkCommandBuffer>& GetCommandBuffers() const;

    private:
        const VkDevice& m_Device;
        VkCommandPool m_CommandPool;
        std::vector<VkCommandBuffer> m_CommandBuffers;
    };
}
