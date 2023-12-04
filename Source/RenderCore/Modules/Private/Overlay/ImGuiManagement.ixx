// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <volk.h>

export module RenderCore.Management.ImGuiManagement;

export import <functional>;

import RenderCore.Management.DeviceManagement;
import RenderCore.Management.PipelineManagement;

namespace RenderCore
{
    export void InitializeImGui(GLFWwindow*, PipelineManager&, DeviceManager&);
    export void ReleaseImGuiResources(VkDevice const&);
    export void DrawImGuiFrame(std::function<void()>&&);
}// namespace RenderCore