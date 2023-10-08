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

import <thread>;
import <queue>;
import <string_view>;
import <stdexcept>;
import <unordered_map>;

import Timer.Manager;
import RenderCore.EngineCore;
import RenderCore.Management.DeviceManagement;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import RenderCore.Utils.GLFWCallbacks;
import RenderCore.Types.Camera;

using namespace RenderCore;

GLFWwindow* g_Window {nullptr};

bool InitializeGLFW(std::uint16_t const Width, std::uint16_t const Height, std::string_view const Title)
{
    if (glfwInit() == 0)
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

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

    glfwSetInputMode(g_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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

bool InitializeImGui()
{
    try
    {
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGuiIO& IO                  = ImGui::GetIO();
        VkExtent2D const DisplaySize = GetWindowExtent(
                g_Window,
                GetAvailablePhysicalDeviceSurfaceCapabilities());

        IO.DisplaySize = ImVec2(static_cast<float>(DisplaySize.width), static_cast<float>(DisplaySize.height));
    }
    catch (std::exception const& Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
        return false;
    }

    return true;
}

Window::Window()
    : m_DrawTimerID(0U),
      m_EventIDQueue(),
      m_MainThreadID(std::this_thread::get_id())
{
}

Window::~Window()
{
    if (!IsInitialized())
    {
        return;
    }

    try
    {
        Shutdown();
        ImGui::DestroyContext();

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
        if (InitializeGLFW(Width, Height, Title) && InitializeEngineCore() && InitializeImGui())
        {
            RegisterTimers();
        }
    }
    catch (std::exception const& Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
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

    Timer::Manager::Get().StopTimer(m_DrawTimerID);
    while (!m_EventIDQueue.empty())
    {
        m_EventIDQueue.pop();
    }

    ShutdownEngine();
}

bool Window::IsInitialized() const
{
    return IsOpen() && IsEngineInitialized();
}

bool Window::IsOpen() const
{
    return g_Window != nullptr && glfwWindowShouldClose(g_Window) == 0;
}

void Window::PollEvents()
{
    if (!IsInitialized() || m_MainThreadID != std::this_thread::get_id())
    {
        return;
    }

    try
    {
        glfwPollEvents();

        std::unordered_map<ApplicationEventFlags, std::uint8_t> ProcessedEvents;

        for (std::uint8_t Iterator = 0U; Iterator < static_cast<std::underlying_type_t<ApplicationEventFlags>>(ApplicationEventFlags::MAX); ++Iterator)
        {
            ProcessedEvents.emplace(static_cast<ApplicationEventFlags>(Iterator), 0U);
        }

        while (!m_EventIDQueue.empty())
        {
            auto const EventFlags = static_cast<ApplicationEventFlags>(m_EventIDQueue.front());
            m_EventIDQueue.pop();

            switch (EventFlags)
            {
                case ApplicationEventFlags::DRAW_FRAME: {
                    if (ProcessedEvents.at(EventFlags) > 0U)
                    {
                        break;
                    }

                    static double DeltaTime = glfwGetTime();
                    DeltaTime               = glfwGetTime() - DeltaTime;

                    GetViewportCamera().UpdateCameraMovement(g_Window, static_cast<float>(DeltaTime));

                    if (ImGuiIO& IO = ImGui::GetIO();
                        IO.Fonts->IsBuilt())
                    {
                        ImGui::NewFrame();
                        {
                            CreateOverlay();
                        }
                        ImGui::Render();
                    }

                    DrawFrame(g_Window);
                    break;
                }
                case ApplicationEventFlags::LOAD_SCENE: {
                    LoadScene(DEBUG_MODEL_OBJ, DEBUG_MODEL_TEX);
                    break;
                }
                case ApplicationEventFlags::UNLOAD_SCENE: {
                    UnloadScene();
                    break;
                }
                default:
                    break;
            }

            ++ProcessedEvents.at(EventFlags);
        }
    }
    catch (std::exception const& Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
    }
}

void Window::CreateOverlay()
{
    ImGui::ShowDemoWindow();
}

void Window::RegisterTimers()
{
    Timer::Manager::Get().SetTickInterval(std::chrono::milliseconds(1));

    {
        // Draw Frame
        constexpr Timer::Parameters DrawFrameTimer {
                .EventID     = static_cast<std::uint8_t>(ApplicationEventFlags::DRAW_FRAME),
                .Interval    = 1000U / g_FrameRate,
                .RepeatCount = std::nullopt};

        m_DrawTimerID = Timer::Manager::Get().StartTimer(DrawFrameTimer, m_EventIDQueue);
    }

    {
        constexpr Timer::Parameters LoadSceneTimer {
                .EventID     = static_cast<std::uint8_t>(ApplicationEventFlags::LOAD_SCENE),
                .Interval    = 1000U,
                .RepeatCount = 0U};

        auto const Discard = Timer::Manager::Get().StartTimer(LoadSceneTimer, m_EventIDQueue);
    }

    if constexpr (false)
    {
        // Unload Scene: Testing Only
        constexpr Timer::Parameters UnLoadSceneTimer {
                .EventID     = static_cast<std::uint8_t>(ApplicationEventFlags::UNLOAD_SCENE),
                .Interval    = 10000U,
                .RepeatCount = 0U};

        auto const Discard = Timer::Manager::Get().StartTimer(UnLoadSceneTimer, m_EventIDQueue);
    }
}