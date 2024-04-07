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
    void WaitAndResetFences();

    void CreateSynchronizationObjects();

    void ReleaseSynchronizationObjects();

    [[nodiscard]] VkSemaphore const &GetImageAvailableSemaphore();

    [[nodiscard]] VkSemaphore const &GetRenderFinishedSemaphore();

    [[nodiscard]] VkFence const &GetFence();
} // namespace RenderCore
