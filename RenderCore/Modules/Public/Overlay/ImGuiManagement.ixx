// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <functional>
#include <GLFW/glfw3.h>

export module RenderCore.Management.ImGuiManagement;

import RenderCore.Management.BufferManagement;
import RenderCore.Management.PipelineManagement;
import RenderCore.Types.SurfaceProperties;

namespace RenderCore
{
    export void InitializeImGuiContext(GLFWwindow*, SurfaceProperties const&);

    export void ReleaseImGuiResources();

    export void DrawImGuiFrame(std::function<void()>&&, std::function<void()>&&, std::function<void()>&&);

    export void DrawPostImGuiResources();

    export [[nodiscard]] bool IsImGuiInitialized();
}// namespace RenderCore
