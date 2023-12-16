// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <boost/log/trivial.hpp>
#include <imgui.h>
#include <numeric>
#include <volk.h>

module RenderCore.Window;

using namespace RenderCore;

import RenderCore.Renderer;
import RenderCore.Management.DeviceManagement;
import RenderCore.Management.ImGuiManagement;
import RenderCore.Utils.Constants;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.EnumHelpers;
import Timer.ExecutionCounter;

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
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    if (IsInitialized())
    {
        return false;
    }

    try
    {
        return m_GLFWHandler.Initialize(Width, Height, Title, Flags) && m_Renderer.Initialize(m_GLFWHandler.GetWindow());
    }
    catch (std::exception const& Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
        Shutdown();
    }

    return false;
}

void Window::Shutdown()
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

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

Renderer& Window::GetRenderer()
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
        glfwPollEvents();
        m_Renderer.Tick();
        RequestRender();
    }
    catch (std::exception const& Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
    }
}

void Window::CreateOverlay()
{
    // Implement in child classes to create overlay
}

void Window::RequestRender()
{
    if (IsInitialized() && IsOpen())
    {
        DrawImGuiFrame([this] {
            CreateOverlay();
        });
        m_Renderer.GetMutableCamera().UpdateCameraMovement(static_cast<float>(m_Renderer.GetDeltaTime()));
        m_Renderer.DrawFrame(m_GLFWHandler.GetWindow(), m_Renderer.GetCamera());
    }
}