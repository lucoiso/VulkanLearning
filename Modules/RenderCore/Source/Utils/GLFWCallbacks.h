// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

#pragma once

#include "Managers/VulkanDeviceManager.h"
#include "VulkanRenderCore.h"
#include <GLFW/glfw3.h>
#include <boost/log/trivial.hpp>

namespace RenderCore
{
    static void GLFWWindowCloseRequested(GLFWwindow* const Window)
    {
        VulkanRenderCore::Get().Shutdown();
        glfwSetWindowShouldClose(Window, GLFW_TRUE);
    }

    static void GLFWWindowResized(GLFWwindow* const Window, [[maybe_unused]] std::int32_t const Width, [[maybe_unused]] std::int32_t const Height)
    {
        VulkanDeviceManager::Get().UpdateDeviceProperties(Window);
    }

    static void GLFWErrorCallback(std::int32_t const Error, char const* const Description)
    {
        BOOST_LOG_TRIVIAL(error) << "[" << __func__ << "]: GLFW Error: " << Error << " - " << Description;
    }

    static void GLFWKeyCallback(GLFWwindow* const Window, std::int32_t const Key, [[maybe_unused]] std::int32_t const Scancode, std::int32_t const Action, [[maybe_unused]] int const Mods)
    {
        if (Key == GLFW_KEY_ESCAPE && Action == GLFW_PRESS)
        {
            GLFWWindowCloseRequested(Window);
        }
    }
}// namespace RenderCore
