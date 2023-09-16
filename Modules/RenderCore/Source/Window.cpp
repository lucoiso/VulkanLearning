// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

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
    Impl(const Window &) = delete;
    Impl &operator=(const Window &) = delete;

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
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        if (m_Window = glfwCreateWindow(Width, Height, Title.data(), nullptr, nullptr); !m_Window)
        {
            throw std::runtime_error("Failed to create GLFW Window");
        }

        if (m_Render = std::make_unique<VulkanRender>())
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Window initialized";
            if (m_Render->Initialize(m_Window))
            {
                m_Render->LoadScene(DEBUG_MODEL_OBJ, DEBUG_MODEL_TEX);
                return true;
            }

            return false;
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

        m_Render.reset();

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
        m_Render->DrawFrame(m_Window);
    }

private:
    GLFWwindow *m_Window;
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

    return m_Impl && m_Impl->Initialize(Width, Height, Title);
}

void Window::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

    m_Impl->Shutdown();
}

bool Window::IsInitialized() const
{
    return m_Impl && m_Impl->IsInitialized();
}

bool Window::IsOpen() const
{
    return IsInitialized() && m_Impl->IsOpen();
}

bool Window::ShouldClose() const
{
    return !IsInitialized() || (m_Impl && m_Impl->ShouldClose());
}

void Window::PollEvents()
{
    if (!IsInitialized())
    {
        return;
    }

    m_Impl->PollEvents();
    m_Impl->DrawFrame();
}