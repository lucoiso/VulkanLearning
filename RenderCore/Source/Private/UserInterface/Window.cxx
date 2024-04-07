// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <GLFW/glfw3.h>
#include <chrono>

module RenderCore.UserInterface.Window;

import RenderCore.Renderer;

using namespace RenderCore;

Window::Window()
    : Control(nullptr)
{
}

Window::~Window()
{
    Shutdown();
}

bool Window::Initialize(std::uint16_t const Width, std::uint16_t const Height, std::string_view const Title, InitializationFlags const Flags)
{
    if (IsInitialized())
    {
        return false;
    }

    m_Title  = Title;
    m_Width  = Width;
    m_Height = Height;
    m_Flags  = Flags;

    if (m_GLFWHandler.Initialize(m_Width, m_Height, m_Title, m_Flags) && RenderCore::Initialize(m_GLFWHandler.GetWindow()))
    {
        OnInitialized();
        RefreshResources();

        return true;
    }

    return false;
}

void Window::Shutdown()
{
    DestroyChildren();

    if (IsInitialized())
    {
        RenderCore::Shutdown(m_GLFWHandler.GetWindow());
    }

    if (IsOpen())
    {
        m_GLFWHandler.Shutdown();
    }
}

bool Window::IsInitialized() const
{
    return Renderer::IsInitialized();
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

    glfwPollEvents();
    Draw();
}

double ToNanoSeconds(std::chrono::time_point<std::chrono::system_clock> const &TimePoint)
{
    return static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(TimePoint.time_since_epoch()).count());
}

void Window::Draw()
{
    if (!IsInitialized() || !IsOpen())
    {
        return;
    }

    static auto LastTime    = ToNanoSeconds(std::chrono::system_clock::now());
    auto const  CurrentTime = ToNanoSeconds(std::chrono::system_clock::now());

    if (double const DeltaTime = (CurrentTime - LastTime) / static_cast<double>(std::nano::den);
        DeltaTime >= Renderer::GetFrameRateCap())
    {
        LastTime = CurrentTime;

        if (IsInitialized() && IsOpen())
        {
            DrawFrame(m_GLFWHandler.GetWindow(), DeltaTime, this);
        }
    }
}
