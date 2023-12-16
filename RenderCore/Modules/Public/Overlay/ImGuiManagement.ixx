// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <functional>

export module RenderCore.Management.ImGuiManagement;

import RenderCore.Management.DeviceManagement;
import RenderCore.Management.PipelineManagement;

namespace RenderCore
{
    export void InitializeImGui(GLFWwindow*, PipelineManager&);
    export void ReleaseImGuiResources();
    export void DrawImGuiFrame(std::function<void()>&&);
}// namespace RenderCore