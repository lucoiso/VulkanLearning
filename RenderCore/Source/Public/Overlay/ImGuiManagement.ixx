// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
    #include <GLFW/glfw3.h>
    #include <functional>
#endif

export module RenderCore.Management.ImGuiManagement;

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
import RenderCore.Management.BufferManagement;
import RenderCore.Management.PipelineManagement;
import RenderCore.Types.SurfaceProperties;
#endif

namespace RenderCore
{
#ifdef VULKAN_RENDERER_ENABLE_IMGUI
    export void               InitializeImGuiContext(GLFWwindow *, SurfaceProperties const &);
    export void               ReleaseImGuiResources();
    export void               DrawImGuiFrame(std::function<void()> &&, std::function<void()> &&, std::function<void()> &&);
    export [[nodiscard]] bool IsImGuiInitialized();
#endif
} // namespace RenderCore
