// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.UserInterface.Window;

import RenderCore.Renderer;
import RenderCore.Utils.EnumHelpers;
import RenderCore.Integrations.ImGuiGLFWBackend;

import Timer.Manager;

using namespace RenderCore;

Timer::Manager s_TimerManager {};

Window::Window()
    : Control(nullptr)
{
}

bool Window::Initialize(std::uint16_t const Width, std::uint16_t const Height, strzilla::string_view const Title, InitializationFlags const Flags)
{
    EASY_FUNCTION(profiler::colors::Red);

    if (Renderer::IsInitialized())
    {
        return false;
    }

    m_Title  = Title;
    m_Width  = Width;
    m_Height = Height;
    m_Flags  = Flags;

    if (m_GLFWHandler.Initialize(m_Width, m_Height, m_Title, m_Flags))
    {
        std::binary_semaphore Sync { 1U };

        s_TimerManager.SetupThread("RenderCore Main");

        s_TimerManager.SetTimer(std::chrono::nanoseconds { 0 },
                                [&]
                                {
                                    if (RenderCore::Initialize(m_GLFWHandler.GetWindow(), Flags))
                                    {
                                        OnInitialized();
                                        RefreshResources();
                                    }

                                    Sync.release();
                                    Draw();
                                });

        s_TimerManager.SetActive(true);
        Sync.acquire();

        return true;
    }

    return false;
}

void Window::RequestClose()
{
    m_PendingClose = true;
}

void Window::Shutdown()
{
    EASY_FUNCTION(profiler::colors::Red);

    std::binary_semaphore Sync { 1U };

    s_TimerManager.SetTimer(std::chrono::nanoseconds { 0 },
                            [&]
                            {
                                if (Renderer::IsInitialized())
                                {
                                    RenderCore::Shutdown(this);
                                }

                                Sync.release();
                            });

    Sync.acquire();

    if (IsOpen())
    {
        m_GLFWHandler.Shutdown();
    }

    s_TimerManager.ClearTimers();
    s_TimerManager.SetActive(false);
}

bool Window::IsOpen() const
{
    return m_GLFWHandler.IsOpen();
}

void Window::PollEvents()
{
    if (!IsOpen())
    {
        return;
    }

    glfwWaitEvents();

    {
        std::queue<std::function<void()>> &DispatchQueue = GetMainThreadDispatchQueue();

        while (!DispatchQueue.empty())
        {
            auto &Dispatch = DispatchQueue.front();
            Dispatch();
            DispatchQueue.pop();
        }
    }

    if (m_PendingClose)
    {
        Shutdown();
    }
}

void Window::Draw()
{
    if (!IsOpen() || m_PendingClose)
    {
        glfwPostEmptyEvent();
        return;
    }

    static auto LastTime    = std::chrono::high_resolution_clock::now();
    auto const  CurrentTime = std::chrono::high_resolution_clock::now();

    auto const     Milliseconds = std::chrono::duration<double, std::milli>(CurrentTime - LastTime).count();
    constexpr auto Denominator  = static_cast<double>(std::milli::den);

    double const &Interval = Renderer::GetFPSLimit();

    if (auto const DeltaTime = Milliseconds / Denominator;
        DeltaTime >= Interval)
    {
        EASY_FUNCTION(profiler::colors::Red);

        LastTime = CurrentTime;

        if (RenderCore::Renderer::IsInitialized())
        {
            DrawFrame(m_GLFWHandler.GetWindow(), DeltaTime, this);
        }
        else
        {
            DestroyChildren();
        }
    }

    s_TimerManager.SetTimer(std::chrono::nanoseconds { 0 },
                            [&]
                            {
                                Draw();
                            });
}
