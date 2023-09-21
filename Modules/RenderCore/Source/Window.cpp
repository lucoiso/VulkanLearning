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
#include <QTimer>

using namespace RenderCore;

class Window::Renderer
{
public:
    Renderer(const Window::Renderer &) = delete;
    Renderer &operator=(const Window::Renderer &) = delete;

    Renderer()
        : m_Render(std::make_unique<VulkanRender>())
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

    bool Initialize(const QWindow *const Window)
    {
        if (IsInitialized())
        {
            return false;
        }

        try
        {
            m_Render->Initialize(Window);
            m_Render->LoadScene(DEBUG_MODEL_OBJ, DEBUG_MODEL_TEX);
            
            return m_Render->IsInitialized();
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

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down Window Renderer";

        m_Render.reset();
    }

    bool IsInitialized() const
    {
        return m_Render && m_Render->IsInitialized();
    }

    void DrawFrame(const QWindow *const Window)
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
    setMinimumSize(Width, Height);
    show();

    if (m_Renderer->Initialize(windowHandle()))
    {
        constexpr std::uint32_t FrameRate = 60u;
        QTimer *const Timer = new QTimer(this);
        connect(Timer, &QTimer::timeout, this, &Window::DrawFrame);
        Timer->start(static_cast<std::int32_t>(1000u / FrameRate));
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

    m_Renderer->Shutdown();
}

bool Window::IsInitialized() const
{
    return m_Renderer && m_Renderer->IsInitialized();
}

void Window::DrawFrame()
{
    try
    {
        m_Renderer->DrawFrame(windowHandle());
    }
    catch (const std::exception &Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
    }
}

bool Window::event(QEvent *const Event)
{
    return QMainWindow::event(Event);
}
