// Copyright Notice: [...]

#include "Window.h"
#include "VulkanRender.h"
#include <stdexcept>
#include <boost/log/trivial.hpp>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

using namespace RenderCore;

class Window::Impl
{
public:
    Impl(const Window&) = delete;
    Impl& operator=(const Window&) = delete;

    Impl()
        : m_Window(nullptr)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Window Implementation";
    }

    ~Impl()
    {
        if (!IsInitialized())
        {
            return;
        }

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing Window Implementation";
        Shutdown();
    }

    bool Initialize(const std::uint16_t Width, const std::uint16_t Height, const std::string_view Title)
    {
        if (IsInitialized())
        {
            return false;
        }

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Initializing Window";

        if (!glfwInit())
        {
            throw std::runtime_error("Failed to initialize GLFW Library");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        if (m_Window = glfwCreateWindow(Width, Height, Title.data(), nullptr, nullptr); !m_Window)
        {
            throw std::runtime_error("Failed to create GLFW Window");
        }

        if (m_Render = std::make_unique<RenderCore::VulkanRender>();
            m_Render)
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Window initialized";
            return m_Render->Initialize(m_Window);
        }

        Shutdown();

        return false;
    }

    void Shutdown()
    {
        if (!IsInitialized())
        {
            return;
        }

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down Window";

        m_Render->Shutdown();

        glfwDestroyWindow(m_Window);
        glfwTerminate();

        m_Window = nullptr;
    }

    bool IsInitialized() const
    {
        return m_Window != nullptr && m_Render != nullptr;
    }

    bool IsOpen() const
    {
        return !ShouldClose();
    }

    bool ShouldClose() const
    {
        return glfwWindowShouldClose(m_Window);
    }

    void PollEvents()
    {
        glfwPollEvents();
    }

    void DrawFrame()
    {
        m_Render->DrawFrame();
    }

private:
    GLFWwindow* m_Window;
    std::unique_ptr<VulkanRender> m_Render;
};

Window::Window()
    : m_Impl(std::make_unique<Window::Impl>())
{
}

Window::~Window()
{
    if (!IsInitialized())
    {
        return;
    }

    Shutdown();
}

bool Window::Initialize(const std::uint16_t Width, const std::uint16_t Height, const std::string_view Title)
{
    if (IsInitialized())
    {
        return false;
    }

    try
    {
        return m_Impl && m_Impl->Initialize(Width, Height, Title);
    }
    catch (const std::exception& Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
    }

    return false;
}

void Window::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

    try
    {
        m_Impl->Shutdown();
    }
    catch (const std::exception& Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
    }
}

bool Window::IsInitialized() const
{
    return m_Impl && m_Impl->IsInitialized();
}

bool Window::IsOpen() const
{
    return m_Impl && m_Impl->IsOpen();
}

bool Window::ShouldClose() const
{
    return m_Impl && m_Impl->ShouldClose();
}

void Window::PollEvents()
{
    if (!IsInitialized())
    {
        return;
    }

    try
    {
        m_Impl->PollEvents();
    }
    catch (const std::exception& Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
    }
}