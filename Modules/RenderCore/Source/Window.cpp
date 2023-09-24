// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "Window.h"
#include "VulkanRender.h"
#include "Utils/VulkanConstants.h"
#include "Utils/RenderCoreHelpers.h"
#include <stdexcept>
#include <boost/log/trivial.hpp>
#include <GLFW/glfw3.h>

using namespace RenderCore;

class Window::Impl
{
public:
    Impl(const Window::Impl &) = delete;
    Impl &operator=(const Window::Impl &) = delete;

    Impl()
        : m_Window(nullptr)
    {
    }

    ~Impl()
    {
        if (!IsInitialized())
        {
            return;
        }

        Shutdown();

        glfwDestroyWindow(m_Window);
        glfwTerminate();
    }

    bool Initialize(const std::uint16_t Width, const std::uint16_t Height, const std::string_view Title)
    {
        if (IsInitialized())
        {
            return false;
        }

        try
        {
            return InitializeGLFW(Width, Height, Title) && InitializeVulkanRender();
        }
        catch (const std::exception &Ex)
        {
            BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
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

        VulkanRender::Get().Shutdown();
    }

    bool IsInitialized() const
    {
        return m_Window && VulkanRender::Get().IsInitialized();
    }

    bool IsOpen() const
    {
        return m_Window && !glfwWindowShouldClose(m_Window);
    }

    void DrawFrame()
    {
        if (!IsInitialized())
        {
            return;
        }

        VulkanRender::Get().DrawFrame(m_Window);
    }

private:
    bool InitializeGLFW(const std::uint16_t Width, const std::uint16_t Height, const std::string_view Title)
    {
        if (!glfwInit())
        {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        if (m_Window = glfwCreateWindow(Width, Height, Title.data(), nullptr, nullptr); !m_Window)
        {
            throw std::runtime_error("Failed to create GLFW Window");
        }

        return m_Window != nullptr;
    }

    bool InitializeVulkanRender()
    {
        VulkanRender::Get().Initialize(m_Window);

        if (VulkanDeviceManager::Get().UpdateDeviceProperties(m_Window))
        {
            VulkanRender::Get().LoadScene(DEBUG_MODEL_OBJ, DEBUG_MODEL_TEX);        
            return VulkanRender::Get().IsInitialized();
        }

        return false;
    }

    GLFWwindow *m_Window;
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

    if (m_Impl->Initialize(Width, Height, Title))
    {
        CreateOverlay();
    }
    else
    {
        Shutdown();
    }

    return IsInitialized();
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
    return m_Impl && m_Impl->IsOpen();
}

void Window::PollEvents()
{
    try
    {
        glfwPollEvents();
        m_Impl->DrawFrame();
    }
    catch (const std::exception &Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
    }
}