// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#pragma once

#include <volk.h>

export module RenderCore.Management.CommandsManagement;

import <optional>;
import <cstdint>;

namespace RenderCore
{
    export void ReleaseCommandsResources();

    export [[nodiscard]] VkCommandPool CreateCommandPool(std::uint8_t);

    export void CreateCommandsSynchronizationObjects();
    export void DestroyCommandsSynchronizationObjects();

    export [[nodiscard]] std::optional<std::int32_t> RequestSwapChainImage();

    export void RecordCommandBuffers(std::uint32_t);
    export void SubmitCommandBuffers();
    export void PresentFrame(std::uint32_t);
}// namespace RenderCore