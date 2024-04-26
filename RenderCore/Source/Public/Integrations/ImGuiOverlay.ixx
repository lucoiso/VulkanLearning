// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <GLFW/glfw3.h>
#include <Volk/volk.h>

export module RenderCore.Integrations.ImGuiOverlay;

import RenderCore.UserInterface.Control;
import RenderCore.Types.SurfaceProperties;
import RenderCore.Types.Allocation;

namespace RenderCore
{
    export void               InitializeImGuiContext(GLFWwindow *);
    export void               ReleaseImGuiResources();
    export void               DrawImGuiFrame(Control *);
    export [[nodiscard]] bool IsImGuiInitialized();
    export void               RecordImGuiCommandBuffer(VkCommandBuffer const &, ImageAllocation const &, VkImageLayout);
} // namespace RenderCore
