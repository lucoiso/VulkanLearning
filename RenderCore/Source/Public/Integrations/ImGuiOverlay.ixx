// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <GLFW/glfw3.h>
#include <functional>
#include <Volk/volk.h>

export module RenderCore.Integrations.ImGuiOverlay;

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
import RenderCore.Types.SurfaceProperties;

namespace RenderCore
{
    export void               InitializeImGuiContext(GLFWwindow *, SurfaceProperties const &);
    export void               ReleaseImGuiResources();
    export void               DrawImGuiFrame(std::function<void()> &&, std::function<void()> &&, std::function<void()> &&);
    export [[nodiscard]] bool IsImGuiInitialized();
    export void               RecordImGuiCommandBuffer(VkCommandBuffer const &, std::uint32_t, VkImageLayout);
} // namespace RenderCore
#endif
