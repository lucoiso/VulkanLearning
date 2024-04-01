// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <Volk/volk.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

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
