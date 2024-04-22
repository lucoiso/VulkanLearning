// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <cstdint>
#include <vector>
#include <Volk/volk.h>

export module RenderCore.Runtime.Command;

import RenderCore.Types.Object;
import RenderCore.Types.Camera;

export namespace RenderCore
{
    void AllocateCommandBuffers(std::uint32_t, std::uint32_t);

    void FreeCommandBuffers();

    void InitializeCommandsResources(std::uint32_t);

    void ReleaseCommandsResources();

    void ReleaseThreadCommandsResources();

    [[nodiscard]] VkCommandPool CreateCommandPool(std::uint8_t, VkCommandPoolCreateFlags);

    void RecordCommandBuffers(std::uint32_t);

    void SubmitCommandBuffers(std::uint32_t);

    void InitializeSingleCommandQueue(VkCommandPool &, std::vector<VkCommandBuffer> &, std::uint8_t);

    void FinishSingleCommandQueue(VkQueue const &, VkCommandPool const &, std::vector<VkCommandBuffer> const &);
} // namespace RenderCore
