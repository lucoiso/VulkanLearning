// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <GLFW/glfw3.h>
#include <boost/log/trivial.hpp>
#include <glm/ext.hpp>
#include <imgui.h>
#include <volk.h>

module RenderCore.Window;

import <thread>;
import <mutex>;
import <queue>;
import <string_view>;
import <stdexcept>;
import <unordered_map>;

import Timer.Manager;
import RenderCore.EngineCore;
import RenderCore.Management.DeviceManagement;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;

using namespace RenderCore;

static GLFWwindow* s_Window {nullptr};

void GLFWWindowCloseRequested(GLFWwindow* const Window)
{
    ShutdownEngine();
    glfwSetWindowShouldClose(Window, GLFW_TRUE);
}

void GLFWWindowResized(GLFWwindow* const Window, [[maybe_unused]] std::int32_t const Width, [[maybe_unused]] std::int32_t const Height)
{
    UpdateDeviceProperties(Window);
    ImGui::GetIO().DisplaySize             = ImVec2(static_cast<float>(Width), static_cast<float>(Height));
    ImGui::GetIO().DisplayFramebufferScale = ImVec2(1.F, 1.F);
    ImGui::GetIO().DeltaTime               = static_cast<float>(glfwGetTime());
}

void GLFWErrorCallback(std::int32_t const Error, char const* const Description)
{
    BOOST_LOG_TRIVIAL(error) << "[" << __func__ << "]: GLFW Error: " << Error << " - " << Description;
}

void GLFWKeyCallback(GLFWwindow* const Window, std::int32_t const Key, [[maybe_unused]] std::int32_t const Scancode, std::int32_t const Action, [[maybe_unused]] int const Mods)
{
    if (Key == GLFW_KEY_ESCAPE && Action == GLFW_PRESS)
    {
        GLFWWindowCloseRequested(Window);
    }

    const auto IsKeyPressed = [](std::int32_t const Key) {
        return glfwGetKey(s_Window, Key) == GLFW_PRESS;
    };

    glm::mat4 Matrix = GetCameraMatrix();

    if (IsKeyPressed(GLFW_KEY_W) || IsKeyPressed(GLFW_KEY_V))
    {
        Matrix[3][2] += s_KeyCallbackRate;
    }

    if (IsKeyPressed(GLFW_KEY_S) || IsKeyPressed(GLFW_KEY_C))
    {
        Matrix[3][2] -= s_KeyCallbackRate;
    }

    if (IsKeyPressed(GLFW_KEY_A) || IsKeyPressed(GLFW_KEY_LEFT))
    {
        Matrix[3][0] += s_KeyCallbackRate;
    }

    if (IsKeyPressed(GLFW_KEY_D) || IsKeyPressed(GLFW_KEY_RIGHT))
    {
        Matrix[3][0] -= s_KeyCallbackRate;
    }

    if (IsKeyPressed(GLFW_KEY_SPACE) || IsKeyPressed(GLFW_KEY_UP))
    {
        Matrix[3][1] -= s_KeyCallbackRate;
    }

    if (IsKeyPressed(GLFW_KEY_LEFT_CONTROL) || IsKeyPressed(GLFW_KEY_DOWN))
    {
        Matrix[3][1] += s_KeyCallbackRate;
    }

    SetCameraMatrix(Matrix);
}

void GLFWCursorPositionCallback(GLFWwindow* Window, double NewCursorPosX, double NewCursorPosY)
{
    static bool StartOperation {false};
    static double CursorPosX {NewCursorPosX};
    static double CursorPosY {NewCursorPosY};

    std::int32_t const LeftButtonEvent   = glfwGetMouseButton(Window, GLFW_MOUSE_BUTTON_LEFT);
    std::int32_t const MiddleButtonEvent = glfwGetMouseButton(Window, GLFW_MOUSE_BUTTON_MIDDLE);
    std::int32_t const RightButtonEvent  = glfwGetMouseButton(Window, GLFW_MOUSE_BUTTON_RIGHT);

    if (LeftButtonEvent == GLFW_PRESS || MiddleButtonEvent == GLFW_PRESS || RightButtonEvent == GLFW_PRESS)
    {
        if (!StartOperation)
        {
            CursorPosX     = NewCursorPosX;
            CursorPosY     = NewCursorPosY;
            StartOperation = true;
        }
        else
        {
            glm::mat4 Matrix = GetCameraMatrix();

            double const OffsetX = NewCursorPosX - CursorPosX;
            double const OffsetY = NewCursorPosY - CursorPosY;

            if (LeftButtonEvent == GLFW_PRESS)
            {
                Matrix = glm::rotate(Matrix, static_cast<float>(OffsetX * s_CursorCallbackRate), glm::vec3(0.F, 0.F, 1.F));
                Matrix = glm::rotate(Matrix, static_cast<float>(OffsetY * s_CursorCallbackRate), glm::vec3(0.F, 1.F, 0.F));
            }

            if (MiddleButtonEvent == GLFW_PRESS || RightButtonEvent == GLFW_PRESS)
            {
                Matrix[3][0] += static_cast<float>(OffsetX * s_CursorCallbackRate);
                Matrix[3][1] -= static_cast<float>(OffsetY * s_CursorCallbackRate);
            }

            CursorPosX = NewCursorPosX;
            CursorPosY = NewCursorPosY;

            SetCameraMatrix(Matrix);
        }
    }
    else
    {
        CursorPosX     = 0.0;
        CursorPosY     = 0.0;
        StartOperation = false;
    }
}

void GLFWCursorScrollCallback([[maybe_unused]] GLFWwindow* const Window, [[maybe_unused]] double const OffsetX, double const OffsetY)
{
    glm::mat4 Matrix = GetCameraMatrix();
    Matrix[3][2] += static_cast<float>(OffsetY * s_KeyCallbackRate);
    SetCameraMatrix(Matrix);
}

bool InitializeGLFW(std::uint16_t const Width, std::uint16_t const Height, std::string_view const Title)
{
    if (glfwInit() == 0)
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    if (s_Window = glfwCreateWindow(Width, Height, Title.data(), nullptr, nullptr);
        s_Window == nullptr)
    {
        throw std::runtime_error("Failed to create GLFW Window");
    }

    glfwSetWindowCloseCallback(s_Window, &GLFWWindowCloseRequested);
    glfwSetWindowSizeCallback(s_Window, &GLFWWindowResized);
    glfwSetKeyCallback(s_Window, &GLFWKeyCallback);
    glfwSetCursorPosCallback(s_Window, &GLFWCursorPositionCallback);
    glfwSetScrollCallback(s_Window, &GLFWCursorScrollCallback);
    glfwSetErrorCallback(&GLFWErrorCallback);

    return s_Window != nullptr;
}

bool InitializeEngineCore()
{
    InitializeEngine(s_Window);

    if (UpdateDeviceProperties(s_Window))
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
                s_Window,
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
      m_Mutex(),
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

        glfwDestroyWindow(s_Window);
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

    std::lock_guard const Lock(m_Mutex);

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
    return s_Window != nullptr && glfwWindowShouldClose(s_Window) == 0;
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

        std::lock_guard const Lock(m_Mutex);
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

                    if (ImGuiIO& IO = ImGui::GetIO();
                        IO.Fonts->IsBuilt())
                    {
                        ImGui::NewFrame();
                        {
                            CreateOverlay();
                        }
                        ImGui::Render();
                    }

                    DrawFrame(s_Window);
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
                .RepeatCount = 0};

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