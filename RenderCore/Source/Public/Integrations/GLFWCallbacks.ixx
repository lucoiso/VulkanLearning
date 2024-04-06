// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <GLFW/glfw3.h>
#include <cstdint>

export module RenderCore.Integrations.GLFWCallbacks;

namespace RenderCore
{
    export void GLFWWindowCloseRequested(GLFWwindow *);
    export void GLFWWindowResized(GLFWwindow *, std::int32_t, std::int32_t);
    export void GLFWErrorCallback(std::int32_t, char const *);
    export void GLFWKeyCallback(GLFWwindow *, std::int32_t, std::int32_t, std::int32_t, std::int32_t);
    export void GLFWCursorPositionCallback(GLFWwindow *, double, double);
    export void GLFWCursorScrollCallback(GLFWwindow *, double, double);
    export void InstallGLFWCallbacks(GLFWwindow *, bool);
} // namespace RenderCore
