// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.UserInterface.Window;

import RenderCore.Renderer;
import RenderCore.Utils.EnumHelpers;

using namespace RenderCore;

Window::Window() : Control(nullptr)
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

    if (m_GLFWHandler.Initialize(m_Width, m_Height, m_Title, m_Flags) && RenderCore::Initialize(m_GLFWHandler.GetWindow(), Flags))
    {
        OnInitialized();
        RefreshResources();

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

    if (Renderer::IsInitialized())
    {
        RenderCore::Shutdown(this);
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
    EASY_FUNCTION(profiler::colors::Red);

    if (!IsOpen())
    {
        return;
    }

    glfwPollEvents();

    if (RenderCore::Renderer::IsInitialized())
    {
        Draw();
    }
    else
    {
        DestroyChildren(true);
    }

    if (m_PendingClose)
    {
        Shutdown();
    }
}

void Window::Draw()
{
    static auto LastTime    = std::chrono::high_resolution_clock::now();
    auto const  CurrentTime = std::chrono::high_resolution_clock::now();

    auto const     Milliseconds = std::chrono::duration<double, std::milli>(CurrentTime - LastTime).count();
    constexpr auto Denominator  = static_cast<double>(std::milli::den);

    if (auto const DeltaTime = static_cast<double>(Milliseconds) / Denominator; DeltaTime >= Renderer::GetFPSLimit())
    {
        LastTime = CurrentTime;
        DrawFrame(m_GLFWHandler.GetWindow(), DeltaTime, this);
    }
}
