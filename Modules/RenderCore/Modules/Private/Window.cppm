// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <boost/log/trivial.hpp>
#include <glm/ext.hpp>
#include <imgui.h>
#include <volk.h>

module RenderCore.Window;

import <chrono>;
import <thread>;
import <queue>;
import <string_view>;
import <stdexcept>;
import <unordered_map>;

import Timer.Manager;
import RenderCore.EngineCore;
import RenderCore.Management.DeviceManagement;
import RenderCore.Management.ImGuiManagement;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import RenderCore.Utils.GLFWCallbacks;
import RenderCore.Types.Camera;
import RenderCore.Types.DeviceProperties;

using namespace RenderCore;

GLFWwindow* g_Window {nullptr};
std::unique_ptr<Timer::Manager> g_RenderTimerManager {std::make_unique<Timer::Manager>()};
bool g_ActiveRender {false};

bool InitializeGLFW(std::uint16_t const Width, std::uint16_t const Height, std::string_view const Title)
{
    if (glfwInit() == 0)
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    if (g_Window = glfwCreateWindow(Width, Height, Title.data(), nullptr, nullptr);
        g_Window == nullptr)
    {
        throw std::runtime_error("Failed to create GLFW Window");
    }

    glfwSetWindowCloseCallback(g_Window, &GLFWWindowCloseRequested);
    glfwSetWindowSizeCallback(g_Window, &GLFWWindowResized);
    glfwSetKeyCallback(g_Window, &GLFWKeyCallback);
    glfwSetCursorPosCallback(g_Window, &GLFWCursorPositionCallback);
    glfwSetScrollCallback(g_Window, &GLFWCursorScrollCallback);
    glfwSetErrorCallback(&GLFWErrorCallback);

    return g_Window != nullptr;
}

bool InitializeEngineCore()
{
    InitializeEngine(g_Window);

    if (UpdateDeviceProperties(g_Window))
    {
        return IsEngineInitialized();
    }

    return false;
}

Window::Window() = default;

Window::~Window()
{
    if (!IsInitialized())
    {
        return;
    }

    try
    {
        Shutdown();

        while (g_RenderTimerManager)
        {
            /* Wait */
        }

        glfwDestroyWindow(g_Window);
        glfwTerminate();
    }
    catch (...)
    {
    }
}

bool Window::Initialize(std::uint16_t const Width, std::uint16_t const Height, std::string_view const Title)
{
    if (IsInitialized())
    {
        return false;
    }

    try
    {
        if (InitializeGLFW(Width, Height, Title) && InitializeEngineCore())
        {
            g_ActiveRender = true;
            g_RenderTimerManager->SetTimer(
                    0U,
                    [this]() {
                        auto const _ = LoadScene(DEBUG_MODEL_OBJ, DEBUG_MODEL_TEX);
                        RequestRender();
                    });

            return true;
        }
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
    if (!IsInitialized())
    {
        return;
    }

    g_ActiveRender = false;
}

bool Window::IsInitialized()
{
    return IsOpen() && IsEngineInitialized();
}

bool Window::IsOpen()
{
    return g_Window != nullptr && glfwWindowShouldClose(g_Window) == 0;
}

void Window::PollEvents() const
{
    if (!IsInitialized())
    {
        return;
    }

    try
    {
        glfwPollEvents();
    }
    catch (std::exception const& Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
    }
}

void Window::CreateOverlay()
{
    constexpr ImGuiWindowFlags OverlayWindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    ImGui::Begin("Vulkan Renderer Options",
                 nullptr,
                 OverlayWindowFlags);

    ImGui::Text("Frame Rate: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
    ImGui::Text("Camera Position: %.3f, %.3f, %.3f", GetViewportCamera().GetPosition().x, GetViewportCamera().GetPosition().y, GetViewportCamera().GetPosition().z);
    ImGui::Text("Camera Yaw: %.3f", GetViewportCamera().GetYaw());
    ImGui::Text("Camera Pitch: %.3f", GetViewportCamera().GetPitch());
    ImGui::Text("Camera Movement State: %d", static_cast<std::underlying_type_t<CameraMovementStateFlags>>(GetViewportCamera().GetCameraMovementStateFlags()));

    ImGui::End();
}

void Window::RequestRender()
{
    if (g_ActiveRender)
    {
        static double DeltaTime = glfwGetTime();
        DeltaTime               = glfwGetTime() - DeltaTime;

        GetViewportCamera().UpdateCameraMovement(g_Window, static_cast<float>(DeltaTime));

        DrawImGuiFrame([this]() {
            CreateOverlay();
        });

        DrawFrame(g_Window);

        g_RenderTimerManager->SetTimer(
                1000U / g_FrameRate,
                [this]() {
                    RequestRender();
                });
    }
    else
    {
        g_RenderTimerManager.reset();
        ShutdownEngine();
    }
}