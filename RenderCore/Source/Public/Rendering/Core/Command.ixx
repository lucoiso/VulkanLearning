// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Runtime.Command;

import RenderCore.Types.Object;
import RenderCore.Types.Camera;
import ThreadPool;

namespace RenderCore
{
    export struct ThreadResources
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
    export void                 InitializeSingleCommandQueue(VkCommandPool &, std::vector<VkCommandBuffer> &, std::uint8_t);
    export void                 FinishSingleCommandQueue(VkQueue const &, VkCommandPool const &, std::vector<VkCommandBuffer> const &);

    export ThreadPool::Pool &GetThreadPool();
} // namespace RenderCore
