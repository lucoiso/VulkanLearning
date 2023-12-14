// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <boost/log/trivial.hpp>
#include <GLFW/glfw3.h>

module RenderCore.Window.GLFWHandler;

using namespace RenderCore;

import Timer.ExecutionCounter;
import RenderCore.Input.GLFWCallbacks;

bool GLFWHandler::Initialize(std::uint16_t const Width, std::uint16_t const Height, std::string_view const Title, InitializationFlags const Flags)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Initializing GLFW Handler";

    if (glfwInit() == 0)
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_MAXIMIZED, HasFlag(Flags, InitializationFlags::MAXIMIZED) ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, HasFlag(Flags, InitializationFlags::HEADLESS) ? GLFW_FALSE : GLFW_TRUE);

    m_Window = glfwCreateWindow(Width, Height, std::data(Title), nullptr, nullptr);

    if (m_Window == nullptr)
    {
        throw std::runtime_error("Failed to create GLFW Window");
    }

    glfwSetWindowCloseCallback(m_Window, &GLFWWindowCloseRequested);
    glfwSetWindowSizeCallback(m_Window, &GLFWWindowResized);
    glfwSetKeyCallback(m_Window, &GLFWKeyCallback);
    glfwSetCursorPosCallback(m_Window, &GLFWCursorPositionCallback);
    glfwSetScrollCallback(m_Window, &GLFWCursorScrollCallback);

    return m_Window != nullptr;
}

void GLFWHandler::Shutdown()
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down GLFW Handler";

    glfwSetWindowShouldClose(m_Window, GLFW_TRUE);
    glfwDestroyWindow(m_Window);
    m_Window = nullptr;
    glfwTerminate();
}

[[nodiscard]] GLFWwindow* GLFWHandler::GetWindow() const
{
    return m_Window;
}

[[nodiscard]] bool GLFWHandler::IsOpen() const
{
    return m_Window != nullptr && glfwWindowShouldClose(m_Window) == GLFW_FALSE;
}