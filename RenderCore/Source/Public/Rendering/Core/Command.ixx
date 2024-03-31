// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <Volk/volk.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

export module RenderCore.Runtime.Command;

import RenderCore.Types.Object;
import RenderCore.Types.Camera;

export namespace RenderCore
{
    void ReleaseCommandsResources();

    [[nodiscard]] VkCommandPool CreateCommandPool(std::uint8_t);

    void CreateCommandsSynchronizationObjects();

    void DestroyCommandsSynchronizationObjects();

    [[nodiscard]] std::optional<std::int32_t> RequestSwapChainImage(VkSwapchainKHR const &);

    void RecordCommandBuffers(std::uint32_t, Camera const &, VkExtent2D const &);

    void SubmitCommandBuffers();

    void PresentFrame(std::uint32_t, VkSwapchainKHR const &);

    void InitializeSingleCommandQueue(VkCommandPool &, std::vector<VkCommandBuffer> &, std::uint8_t);

    void FinishSingleCommandQueue(VkQueue const &, VkCommandPool const &, std::vector<VkCommandBuffer> &);
} // namespace RenderCore
