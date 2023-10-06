// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <volk.h>

export module RenderCore.Management.CommandsManagement;

import <optional>;
import <cstdint>;

export namespace RenderCore
{
    void ReleaseCommandsResources();

    [[nodiscard]] VkCommandPool CreateCommandPool(std::uint8_t);

    void CreateCommandsSynchronizationObjects();
    void DestroyCommandsSynchronizationObjects();

    [[nodiscard]] std::optional<std::int32_t> RequestSwapChainImage();

    void RecordCommandBuffers(std::uint32_t);
    void SubmitCommandBuffers();
    void PresentFrame(std::uint32_t);
}// namespace RenderCore