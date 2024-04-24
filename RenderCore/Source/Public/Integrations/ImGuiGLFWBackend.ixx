// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

// Adapted from: https://github.com/ocornut/imgui/blob/docking/backends/imgui_impl_glfw.h

module;

#include <cstdint>
#include <GLFW/glfw3.h>

export module RenderCore.Integrations.ImGuiGLFWBackend;

export namespace RenderCore
{
    void ImGuiGLFWUpdateMonitors();
    void ImGuiGLFWInitPlatformInterface();
    void ImGuiGLFWShutdownPlatformInterface();
    bool ImGuiGLFWInitForVulkan(GLFWwindow *, bool);
    void ImGuiGLFWShutdown();
    void ImGuiGLFWNewFrame();
    void ImGuiGLFWInstallCallbacks(GLFWwindow *);
    void ImGuiGLFWRestoreCallbacks(GLFWwindow *);
    void ImGuiGLFWSetCallbacksChainForAllWindows(bool);
    void ImGuiGLFWWindowFocusCallback(GLFWwindow *, std::int32_t);
    void ImGuiGLFWCursorEnterCallback(GLFWwindow *, std::int32_t);
    void ImGuiGLFWCursorPosCallback(GLFWwindow *, double, double);
    void ImGuiGLFWMouseButtonCallback(GLFWwindow *, std::int32_t, std::int32_t, std::int32_t);
    void ImGuiGLFWScrollCallback(GLFWwindow *, double, double);
    void ImGuiGLFWKeyCallback(GLFWwindow *, std::int32_t, std::int32_t, std::int32_t, std::int32_t);
    void ImGuiGLFWCharCallback(GLFWwindow *, std::uint32_t);
    void ImGuiGLFWMonitorCallback(GLFWmonitor *, std::int32_t);
}
