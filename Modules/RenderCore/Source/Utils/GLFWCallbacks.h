// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "VulkanRenderCore.h"
#include "Managers/VulkanDeviceManager.h"
#include <GLFW/glfw3.h>
#include <boost/log/trivial.hpp>

namespace RenderCore
{
    static inline void GLFWWindowCloseRequested(GLFWwindow* const Window)
    {
        VulkanRenderCore::Get().Shutdown();
        glfwSetWindowShouldClose(Window, GLFW_TRUE);
    }

    static inline void GLFWWindowResized(GLFWwindow* const Window, [[maybe_unused]] const std::int32_t Width, [[maybe_unused]] const std::int32_t Height)
    {
        VulkanDeviceManager::Get().UpdateDeviceProperties(Window);
    }

    static inline void GLFWErrorCallback(const std::int32_t Error, const char* const Description)
    {
        BOOST_LOG_TRIVIAL(error) << "[" << __func__ << "]: GLFW Error: " << Error << " - " << Description;
    }

    static inline void GLFWKeyCallback(GLFWwindow* const Window, const std::int32_t Key, [[maybe_unused]] const std::int32_t Scancode, const std::int32_t Action, [[maybe_unused]] const int Mods)
    {
        if (Key == GLFW_KEY_ESCAPE && Action == GLFW_PRESS)
        {
            GLFWWindowCloseRequested(Window);
        }
    }
};
