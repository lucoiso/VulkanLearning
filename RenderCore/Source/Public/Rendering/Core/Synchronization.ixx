// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <Volk/volk.h>

export module RenderCore.Runtime.Synchronization;

import RenderCore.Types.Object;
import RenderCore.Types.Camera;

export namespace RenderCore
{
    void                      ResetSemaphores();
    void                      ResetFenceStatus();
    void                      SetFenceWaitStatus(std::uint32_t, bool);
    [[nodiscard]] bool const &GetFenceWaitStatus(std::uint32_t);
    void                      WaitAndResetFence(std::uint32_t);
    void                      CreateSynchronizationObjects();
    void                      ReleaseSynchronizationObjects();

    [[nodiscard]] VkSemaphore const &GetImageAvailableSemaphore(std::uint32_t);
    [[nodiscard]] VkSemaphore const &GetRenderFinishedSemaphore(std::uint32_t);
    [[nodiscard]] VkFence const     &GetFence(std::uint32_t);
} // namespace RenderCore
