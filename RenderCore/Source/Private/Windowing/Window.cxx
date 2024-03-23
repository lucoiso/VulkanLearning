// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <chrono>
#include <numeric>
#include <string>
#include <boost/log/trivial.hpp>
#include <GLFW/glfw3.h>

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
#include <imgui.h>
#endif

module RenderCore.Window;

using namespace RenderCore;

import RenderCore.Renderer;
import RenderCore.Management.DeviceManagement;
import RenderCore.Management.ImGuiManagement;
import RenderCore.Utils.Constants;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.EnumHelpers;
import RuntimeInfo.Manager;

Window::Window()
    : Control(nullptr)
{
}

Window::~Window()
{
    try
    {
        Shutdown();
    }
    catch (...)
    {
    }
}

bool Window::Initialize(std::uint16_t const Width, std::uint16_t const Height, std::string_view const Title, InitializationFlags const Flags)
{
    auto const _ { RuntimeInfo::Manager::Get().PushCallstackWithCounter() };

    if (IsInitialized())
    {
        return false;
    }

    m_Title = Title;
    m_Width = Width;
    m_Height = Height;
    m_Flags = Flags;

    try
    {
        if (m_GLFWHandler.Initialize(m_Width, m_Height, m_Title, m_Flags) && m_Renderer.Initialize(m_GLFWHandler.GetWindow()))
        {
            OnInitialized();
            RefreshResources();
            return true;
        }
    }
    catch (std::exception const &Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
        Shutdown();
    }

    return false;
}

void Window::Shutdown()
{
    auto const _ { RuntimeInfo::Manager::Get().PushCallstackWithCounter() };

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

    try
    {
        static std::uint64_t LastTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        std::uint64_t const CurrentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        double const DeltaTime = static_cast<double>(CurrentTime - LastTime) / 1000.0;

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
    catch (std::exception const &Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
    }
}

void Window::RequestRender(float const DeltaTime)
{
    RuntimeInfo::Manager::Get().PushCallstack();

    if (!IsInitialized() || !IsOpen())
    {
        return;
    }

    m_Renderer.GetMutableCamera().UpdateCameraMovement(static_cast<float>(m_Renderer.GetDeltaTime()));
    m_Renderer.DrawFrame(m_GLFWHandler.GetWindow(), DeltaTime, m_Renderer.GetCamera(), this);
}
