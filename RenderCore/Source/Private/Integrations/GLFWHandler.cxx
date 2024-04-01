// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <string_view>

// Include vulkan before glfw
#include <Volk/volk.h>

// Include glfw after vulkan
#include <GLFW/glfw3.h>

module RenderCore.Integrations.GLFWHandler;

using namespace RenderCore;

import RuntimeInfo.Manager;
import RenderCore.Integrations.GLFWCallbacks;
import RenderCore.Utils.EnumHelpers;

bool GLFWHandler::Initialize(std::uint16_t const Width, std::uint16_t const Height, std::string_view const Title, InitializationFlags const Flags)
{
    if (!glfwInit() || !glfwVulkanSupported())
    {
        return false;
    }

    if (static bool GLFWErrorCallbacksSet = false; !GLFWErrorCallbacksSet)
    {
        glfwSetErrorCallback(&GLFWErrorCallback);
        GLFWErrorCallbacksSet = true;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_MAXIMIZED, HasFlag(Flags, InitializationFlags::MAXIMIZED) ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, HasFlag(Flags, InitializationFlags::HEADLESS) ? GLFW_FALSE : GLFW_TRUE);

    m_Window = glfwCreateWindow(Width, Height, std::data(Title), nullptr, nullptr);

    InstallGLFWCallbacks(m_Window, true);

    return m_Window != nullptr;
}

void GLFWHandler::Shutdown()
{
    glfwSetWindowShouldClose(m_Window, GLFW_TRUE);
    glfwDestroyWindow(m_Window);
    m_Window = nullptr;
    glfwTerminate();
}

[[nodiscard]] GLFWwindow *GLFWHandler::GetWindow() const
{
    return m_Window;
}

[[nodiscard]] bool GLFWHandler::IsOpen() const
{
    return m_Window != nullptr && glfwWindowShouldClose(m_Window) == GLFW_FALSE;
}
