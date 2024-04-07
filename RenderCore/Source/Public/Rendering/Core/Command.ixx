// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <Volk/volk.h>
#include <cstdint>
#include <vector>

export module RenderCore.Runtime.Command;

import RenderCore.Types.Object;
import RenderCore.Types.Camera;

export namespace RenderCore
{
    void AllocateCommandBuffers(std::uint32_t, std::uint32_t);

    void FreeCommandBuffers();

    void InitializeCommandsResources();

    void ReleaseCommandsResources();

    [[nodiscard]] VkCommandPool CreateCommandPool(std::uint8_t, VkCommandPoolCreateFlags);

    void RecordCommandBuffers(std::uint32_t);

    void SubmitCommandBuffers();

    void InitializeSingleCommandQueue(VkCommandPool &, std::vector<VkCommandBuffer> &, std::uint8_t);

    void FinishSingleCommandQueue(VkQueue const &, VkCommandPool const &, std::vector<VkCommandBuffer> const &);
} // namespace RenderCore
