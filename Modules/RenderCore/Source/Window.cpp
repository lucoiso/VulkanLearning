// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "Window.h"
#include "VulkanRender.h"
#include <stdexcept>
#include <boost/log/trivial.hpp>
#include <QTimer>
#include <QEvent>
#include <QPluginLoader>

using namespace RenderCore;

class Window::Impl
{
public:
    Impl(const Window::Impl &) = delete;
    Impl &operator=(const Window::Impl &) = delete;

    Impl()
        : m_Render()
    {
    }

    ~Impl()
    {
        if (!IsInitialized())
        {
            return;
        }

        Shutdown();
    }

    bool Initialize(const QQuickWindow *const Window)
    {
        if (IsInitialized())
        {
            return false;
        }

        try
        {
            m_Render.Initialize(Window);
            m_Render.LoadScene(DEBUG_MODEL_OBJ, DEBUG_MODEL_TEX);
            
            return m_Render.IsInitialized();
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

        m_Render.Shutdown();
    }

    bool IsInitialized() const
    {
        return m_Render.IsInitialized();
    }

    void DrawFrame(const QQuickWindow *const Window)
    {
        m_Render.DrawFrame(Window);
    }

private:

    VulkanRender m_Render;
};

Window::Window()
    : QQuickView()
    , m_Impl(std::make_unique<Window::Impl>())
    , m_CanDraw(false)
{
    CreateOverlay();
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

    // setSource(QUrl("qrc:/RenderCore/QML/Placeholder.qml"));
    setResizeMode(QQuickView::SizeRootObjectToView);
    setTitle(Title.data());
    setMinimumSize(QSize(Width, Height));
    show();

    if (m_Impl->Initialize(this))
    {
        m_CanDraw = true;

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

    m_Impl->Shutdown();
}

bool Window::IsInitialized() const
{
    return m_Impl && m_Impl->IsInitialized();
}

void Window::CreateOverlay()
{
}

void Window::DrawFrame()
{
    try
    {
        if (m_CanDraw)
        {
            m_Impl->DrawFrame(this);
        }
    }
    catch (const std::exception &Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
    }
}

bool Window::event(QEvent *const Event)
{
    if (Event->type() == QEvent::Close)
    {
        m_CanDraw = false;
    }

    return QQuickWindow::event(Event);
}
