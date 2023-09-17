// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "Window.h"
#include "VulkanRender.h"
#include "Managers/VulkanRenderSubsystem.h"
#include "Utils/RenderCoreHelpers.h"
#include "Utils/VulkanConstants.h"
#include <stdexcept>
#include <boost/log/trivial.hpp>

using namespace RenderCore;

class Window::Renderer
{
public:
    Renderer(const Window::Renderer &) = delete;
    Renderer &operator=(const Window::Renderer &) = delete;

    Renderer()
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Window Renderer";
    }

    ~Renderer()
    {
        if (!IsInitialized())
        {
            return;
        }

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing Window Renderer";
        Shutdown();
    }

    bool Initialize(QWindow *const Window)
    {
        if (IsInitialized())
        {
            return false;
        }

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Initializing Window Renderer";

        if (m_Render = std::make_unique<VulkanRender>())
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Window Renderer Initialized";

            if (m_Render->Initialize(Window); m_Render->IsInitialized())
            {
                //m_Render->LoadScene(DEBUG_MODEL_OBJ, DEBUG_MODEL_TEX);
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

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down Window Renderer";

        m_Render.reset();
    }

    bool IsInitialized() const
    {
        return m_Render != nullptr;
    }

    void DrawFrame(QWindow *const Window)
    {
        m_Render->DrawFrame(Window);
    }

private:

    std::unique_ptr<VulkanRender> m_Render;
};

Window::Window()
    : QMainWindow()
    , m_Renderer(std::make_unique<Window::Renderer>())
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

    setWindowTitle(Title.data());
    setBaseSize(Width, Height);
    show();

    return m_Renderer && m_Renderer->Initialize(qobject_cast<QWindow*>(window()));
}

void Window::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

    m_Renderer->Shutdown();
}

bool Window::IsInitialized() const
{
    return m_Renderer && m_Renderer->IsInitialized();
}

bool Window::IsOpen() const
{
    return IsInitialized();
}

bool Window::ShouldClose() const
{
    return !IsInitialized();
}

void Window::PollEvents()
{
    if (!IsInitialized())
    {
        return;
    }

    m_Renderer->DrawFrame(qobject_cast<QWindow*>(window()));
}