// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

// Adapted from: https://github.com/ocornut/imgui/blob/docking/backends/imgui_impl_glfw.h

module;

export module RenderCore.Integrations.ImGuiGLFWBackend;

namespace RenderCore
{
    export bool ImGuiGLFWInitForVulkan(GLFWwindow *, bool);
    export void ImGuiGLFWShutdown();
    export void ImGuiGLFWNewFrame();
    export void ImGuiGLFWInstallCallbacks(GLFWwindow *);
    export void ImGuiGLFWRestoreCallbacks(GLFWwindow *);
    export void ImGuiGLFWSetCallbacksChainForAllWindows(bool);

    export void ImGuiGLFWUpdateFrameBufferSizes();
    export void ImGuiGLFWUpdateMouse();

    void ImGuiGLFWUpdateMonitors();
    void ImGuiGLFWInitPlatformInterface();
    void ImGuiGLFWShutdownPlatformInterface();
    void ImGuiGLFWWindowFocusCallback(GLFWwindow *, std::int32_t);
    void ImGuiGLFWCursorEnterCallback(GLFWwindow *, std::int32_t);
    void ImGuiGLFWCursorPosCallback(GLFWwindow *, double, double);
    void ImGuiGLFWMouseButtonCallback(GLFWwindow *, std::int32_t, std::int32_t, std::int32_t);
    void ImGuiGLFWScrollCallback(GLFWwindow *, double, double);
    void ImGuiGLFWKeyCallback(GLFWwindow *, std::int32_t, std::int32_t, std::int32_t, std::int32_t);
    void ImGuiGLFWCharCallback(GLFWwindow *, std::uint32_t);
    void ImGuiGLFWMonitorCallback(GLFWmonitor *, std::int32_t);
}
