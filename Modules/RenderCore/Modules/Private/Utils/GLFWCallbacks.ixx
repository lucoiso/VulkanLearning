// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>

export module RenderCore.Utils.GLFWCallbacks;

export import <cstdint>;

namespace RenderCore
{
    export void GLFWWindowCloseRequested(GLFWwindow*);
    export void GLFWWindowResized(GLFWwindow*, std::int32_t, std::int32_t);
    export void GLFWErrorCallback(std::int32_t, char const*);
    export void GLFWKeyCallback(GLFWwindow*, std::int32_t, std::int32_t, std::int32_t, std::int32_t);
    export void GLFWCursorPositionCallback(GLFWwindow*, double, double);
    export void GLFWCursorScrollCallback(GLFWwindow*, double, double);
}// namespace RenderCore