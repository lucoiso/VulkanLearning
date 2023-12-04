// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <volk.h>

export module RenderCore.Management.CommandsManagement;

export import <optional>;
export import <cstdint>;
export import <memory>;

import RenderCore.Management.BufferManagement;
import RenderCore.Management.PipelineManagement;

namespace RenderCore
{
    export class CommandsManager
    {
        VkCommandPool m_CommandPool {};
        std::vector<VkCommandBuffer> m_CommandBuffers {};
        VkSemaphore m_ImageAvailableSemaphore {};
        VkSemaphore m_RenderFinishedSemaphore {};
        VkFence m_Fence {};

        void AllocateCommandBuffer(VkDevice const&, std::uint32_t);
        void WaitAndResetFences(VkDevice const&) const;
        void FreeCommandBuffers(VkDevice const&);

    public:
        void ReleaseCommandsResources(VkDevice const&);
        [[nodiscard]] VkCommandPool CreateCommandPool(VkDevice const&, std::uint8_t);

        void CreateCommandsSynchronizationObjects(VkDevice const&);
        void DestroyCommandsSynchronizationObjects(VkDevice const&);

        [[nodiscard]] std::optional<std::int32_t> RequestSwapChainImage(VkDevice const&, VkSwapchainKHR const&) const;

        void RecordCommandBuffers(VkDevice const&, std::uint32_t, std::uint32_t, BufferManager&, PipelineManager&, std::vector<std::shared_ptr<Object>> const&);
        void SubmitCommandBuffers(VkDevice const&, VkQueue const&);

        void PresentFrame(VkQueue const&, std::uint32_t, VkSwapchainKHR const&);
    };
    export void InitializeSingleCommandQueue(VkDevice const&, VkCommandPool&, VkCommandBuffer&, std::uint8_t);
    export void FinishSingleCommandQueue(VkDevice const&, VkQueue const&, VkCommandPool const&, VkCommandBuffer const&);
}// namespace RenderCore