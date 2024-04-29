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

namespace RenderCore
{
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
} // namespace RenderCore
