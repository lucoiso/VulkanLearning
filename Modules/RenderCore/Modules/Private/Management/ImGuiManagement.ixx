// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <volk.h>

export module RenderCore.Management.ImGuiManagement;

import <functional>;

namespace RenderCore
{
    export void InitializeImGui(GLFWwindow*);
    export void ReleaseImGuiResources();
    export void DrawImGuiFrame(std::function<void()>&&);
}// namespace RenderCore