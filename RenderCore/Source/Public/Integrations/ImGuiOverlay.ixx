// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <functional>

export module RenderCore.Integrations.ImGuiOverlay;

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
import RenderCore.Types.SurfaceProperties;

namespace RenderCore
{
    export void               InitializeImGuiContext(GLFWwindow *, SurfaceProperties const &);
    export void               ReleaseImGuiResources();
    export void               DrawImGuiFrame(std::function<void()> &&, std::function<void()> &&, std::function<void()> &&);
    export [[nodiscard]] bool IsImGuiInitialized();
} // namespace RenderCore
#endif
