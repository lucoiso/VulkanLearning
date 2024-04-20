// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <chrono>
#include <GLFW/glfw3.h>

module RenderCore.UserInterface.Window;

import RenderCore.Renderer;

using namespace RenderCore;

Window::Window()
    : Control(nullptr)
{
}

bool Window::Initialize(std::uint16_t const Width, std::uint16_t const Height, std::string_view const Title, InitializationFlags const Flags)
{
    if (Renderer::IsInitialized())
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

    if (Renderer::IsInitialized())
    {
        RenderCore::Shutdown(m_GLFWHandler.GetWindow());
    }

    if (IsOpen())
    {
        m_GLFWHandler.Shutdown();
    }
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

void Window::Draw()
{
    static auto LastTime    = std::chrono::high_resolution_clock::now();
    auto const  CurrentTime = std::chrono::high_resolution_clock::now();

    auto const     Milliseconds = std::chrono::duration<double, std::milli>(CurrentTime - LastTime).count();
    constexpr auto Denominator  = static_cast<double>(std::milli::den);

    if (auto const DeltaTime = static_cast<double>(Milliseconds) / Denominator;
        DeltaTime >= Renderer::GetFrameRateCap())
    {
        DrawFrame(m_GLFWHandler.GetWindow(), DeltaTime, this);
    }

    LastTime = CurrentTime;
}
