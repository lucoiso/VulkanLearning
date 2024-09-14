// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Runtime.Command;

import ThreadPool;
import RenderCore.Types.Allocation;

namespace RenderCore
{
    RENDERCOREMODULE_API ThreadPool::Pool g_ThreadPool {};

    std::function<void(std::uint8_t)>                                     g_OnCommandPoolResetCallback {};
    std::function<void(VkCommandBuffer const &, ImageAllocation const &)> g_OnCommandBufferRecordCallback {};

    export struct RENDERCOREMODULE_API ThreadResources
    {
        VkCommandPool   CommandPool { VK_NULL_HANDLE };
        VkCommandBuffer CommandBuffer { VK_NULL_HANDLE };

        void Allocate(VkDevice const &, std::uint8_t);
        void Free(VkDevice const &);
        void Destroy(VkDevice const &);
        void Reset(VkDevice const &);
    };

    [[nodiscard]] VkCommandPool CreateCommandPool(std::uint8_t, VkCommandPoolCreateFlags);
    export void                 SetNumObjectsPerThread(std::uint32_t);
    export void                 ResetCommandPool(std::uint32_t);
    export void                 FreeCommandBuffers();
    export void                 InitializeCommandsResources(std::uint32_t);
    export void                 ReleaseCommandsResources();
    export void                 RecordCommandBuffers(std::uint32_t);
    export void                 SubmitCommandBuffers(std::uint32_t);

    export RENDERCOREMODULE_API void InitializeSingleCommandQueue(VkCommandPool &, std::vector<VkCommandBuffer> &, std::uint8_t);
    export RENDERCOREMODULE_API void FinishSingleCommandQueue(VkQueue const &, VkCommandPool const &, std::vector<VkCommandBuffer> const &);

    export RENDERCOREMODULE_API [[nodiscard]] inline ThreadPool::Pool &GetThreadPool()
    {
        return g_ThreadPool;
    }

    export RENDERCOREMODULE_API inline void SetOnCommandPoolResetCallbackCallback(std::function<void(std::uint8_t)> &&Callback)
    {
        g_OnCommandPoolResetCallback = std::move(Callback);
    }

    export RENDERCOREMODULE_API inline void SetOnCommandBufferRecordCallbackCallback(
            std::function<void(VkCommandBuffer const &, ImageAllocation const &)> &&Callback)
    {
        g_OnCommandBufferRecordCallback = std::move(Callback);
    }
} // namespace RenderCore
