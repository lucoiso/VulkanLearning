// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Integrations.GLFWCallbacks;

namespace RenderCore
{
    export [[nodiscard]] bool IsResizingMainWindow();
    void                      GLFWWindowCloseRequestedCallback(GLFWwindow *);
    void                      GLFWWindowResizedCallback(GLFWwindow *, std::int32_t, std::int32_t);
    export void               GLFWErrorCallback(std::int32_t, char const *);
    void                      GLFWKeyCallback(GLFWwindow *, std::int32_t, std::int32_t, std::int32_t, std::int32_t);
    export void               GLFWCursorPositionCallback(GLFWwindow *, double, double);
    void                      GLFWCursorScrollCallback(GLFWwindow *, double, double);
    export void               InstallGLFWCallbacks(GLFWwindow *, bool);
    export void SetViewportControlsCamera(bool);
    export [[nodiscard]] bool  ViewportControlsCamera();
    export void SetViewportHovering(bool);
} // namespace RenderCore
