// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#ifdef LINK_IMGUI
#include <functional>
#include <GLFW/glfw3.h>
#endif

export module RenderCore.Management.ImGuiManagement;

#ifdef LINK_IMGUI
import RenderCore.Management.BufferManagement;
import RenderCore.Management.PipelineManagement;
import RenderCore.Types.SurfaceProperties;
#endif

namespace RenderCore
{
#ifdef LINK_IMGUI
    export void InitializeImGuiContext(GLFWwindow*, SurfaceProperties const&);
    export void ReleaseImGuiResources();
    export void DrawImGuiFrame(std::function<void()>&&, std::function<void()>&&, std::function<void()>&&);
    export [[nodiscard]] bool IsImGuiInitialized();
#endif
}// namespace RenderCore
