// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <Volk/volk.h>
#include <cstdint>

export module RenderCore.Runtime.Synchronization;

import RenderCore.Types.Object;
import RenderCore.Types.Camera;

export namespace RenderCore
{
    void WaitAndResetFence(std::uint32_t);
    void CreateSynchronizationObjects();
    void ReleaseSynchronizationObjects();
    void ResetSemaphores();

    [[nodiscard]] VkSemaphore const &GetImageAvailableSemaphore();
    [[nodiscard]] VkSemaphore const &GetRenderFinishedSemaphore();
    [[nodiscard]] VkFence const &    GetFence(std::uint32_t);
} // namespace RenderCore
