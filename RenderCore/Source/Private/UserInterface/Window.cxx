// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <boost/log/trivial.hpp>
#include <chrono>
#include "Utils/Library/Macros.h"

module RenderCore.UserInterface.Window;

using namespace RenderCore;

import RenderCore.Renderer;
import RuntimeInfo.Manager;

Window::Window() : Control(nullptr)
{
}

Window::~Window()
{
    Shutdown();
}

bool Window::Initialize(std::uint16_t const Width, std::uint16_t const Height, std::string_view const Title, InitializationFlags const Flags)
{
    PUSH_CALLSTACK_WITH_COUNTER();

    if (IsInitialized())
    {
        return false;
    }

    m_Title  = Title;
    m_Width  = Width;
    m_Height = Height;
    m_Flags  = Flags;

    if (m_GLFWHandler.Initialize(m_Width, m_Height, m_Title, m_Flags) && m_Renderer.Initialize(m_GLFWHandler.GetWindow()))
    {
        OnInitialized();
        RefreshResources();
        return true;
    }

    return false;
}

void Window::Shutdown()
{
    PUSH_CALLSTACK_WITH_COUNTER();

    DestroyChildren();

    if (IsInitialized())
    {
        m_Renderer.Shutdown(m_GLFWHandler.GetWindow());
    }

    if (IsOpen())
    {
        m_GLFWHandler.Shutdown();
    }
}

bool Window::IsInitialized() const
{
    return m_Renderer.IsInitialized();
}

bool Window::IsOpen() const
{
    return m_GLFWHandler.IsOpen();
}

Renderer &Window::GetRenderer()
{
    return m_Renderer;
}

void Window::PollEvents()
{
    if (!IsOpen())
    {
        return;
    }

    static std::uint64_t LastTime    = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::uint64_t const  CurrentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    double const         DeltaTime   = static_cast<double>(CurrentTime - LastTime) / 1000.0;

    if (DeltaTime < GetRenderer().GetFrameRateCap())
    {
        return;
    }

    if (DeltaTime > 0.F)
    {
        LastTime = CurrentTime;
    }

    glfwPollEvents();

    m_Renderer.Tick();
    RequestRender(DeltaTime);
}

void Window::RequestRender(float const DeltaTime)
{
    PUSH_CALLSTACK();

    if (!IsInitialized() || !IsOpen())
    {
        return;
    }

    m_Renderer.GetMutableCamera().UpdateCameraMovement(static_cast<float>(m_Renderer.GetDeltaTime()));
    m_Renderer.DrawFrame(m_GLFWHandler.GetWindow(), DeltaTime, m_Renderer.GetCamera(), this);
}
