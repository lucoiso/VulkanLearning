// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <GLFW/glfw3.h>
#include <cstdint>

export module RenderCore.Integrations.GLFWCallbacks;

namespace RenderCore
{
    void        GLFWWindowCloseRequested(GLFWwindow *);
    void        GLFWWindowResized(GLFWwindow *, std::int32_t, std::int32_t);
    export void GLFWErrorCallback(std::int32_t, char const *);
    void        GLFWKeyCallback(GLFWwindow *, std::int32_t, std::int32_t, std::int32_t, std::int32_t);
    void        GLFWCursorPositionCallback(GLFWwindow *, double, double);
    void        GLFWCursorScrollCallback(GLFWwindow *, double, double);
    export void InstallGLFWCallbacks(GLFWwindow *, bool);
} // namespace RenderCore
