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
import RenderCore.Managers.DeviceManager;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;

using namespace RenderCore;

static GLFWwindow* s_Window {nullptr};

static void GLFWWindowCloseRequested(GLFWwindow* const Window)
{
    EngineCore::Get().Shutdown();
    glfwSetWindowShouldClose(Window, GLFW_TRUE);
}

static void GLFWWindowResized(GLFWwindow* const Window, [[maybe_unused]] std::int32_t const Width, [[maybe_unused]] std::int32_t const Height)
{
    DeviceManager::Get().UpdateDeviceProperties(Window);
    ImGui::GetIO().DisplaySize             = ImVec2(static_cast<float>(Width), static_cast<float>(Height));
    ImGui::GetIO().DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    ImGui::GetIO().DeltaTime               = static_cast<float>(glfwGetTime());
}

static void GLFWErrorCallback(std::int32_t const Error, char const* const Description)
{
    BOOST_LOG_TRIVIAL(error) << "[" << __func__ << "]: GLFW Error: " << Error << " - " << Description;
}

static void GLFWKeyCallback(GLFWwindow* const Window, std::int32_t const Key, [[maybe_unused]] std::int32_t const Scancode, std::int32_t const Action, [[maybe_unused]] int const Mods)
{
    if (Key == GLFW_KEY_ESCAPE && Action == GLFW_PRESS)
    {
        GLFWWindowCloseRequested(Window);
    }

    if (Action != GLFW_RELEASE)
    {
        constexpr float TickRate = g_FrameRate / 1000.f;
        glm::mat4 Matrix         = EngineCore::Get().GetCameraMatrix();

        if (Key == GLFW_KEY_W || Key == GLFW_KEY_V)
        {
            Matrix[3][2] += TickRate;
        }

        if (Key == GLFW_KEY_S || Key == GLFW_KEY_C)
        {
            Matrix[3][2] -= TickRate;
        }

        if (Key == GLFW_KEY_A || Key == GLFW_KEY_LEFT)
        {
            Matrix[3][0] += TickRate;
        }

        if (Key == GLFW_KEY_D || Key == GLFW_KEY_RIGHT)
        {
            Matrix[3][0] -= TickRate;
        }

        if (Key == GLFW_KEY_SPACE || Key == GLFW_KEY_UP)
        {
            Matrix[3][1] -= TickRate;
        }

        if (Key == GLFW_KEY_LEFT_CONTROL || Key == GLFW_KEY_DOWN)
        {
            Matrix[3][1] += TickRate;
        }

        EngineCore::Get().SetCameraMatrix(Matrix);
    }
}

static void GLFWCursorPositionCallback(GLFWwindow* Window, double NewCursorPosX, double NewCursorPosY)
{
    static bool StartOperation {false};
    static double CursorPosX {NewCursorPosX};
    static double CursorPosY {NewCursorPosY};

    if (std::int32_t const ButtonEvent = glfwGetMouseButton(Window, GLFW_MOUSE_BUTTON_LEFT); ButtonEvent == GLFW_PRESS)
    {
        if (!StartOperation)
        {
            CursorPosX     = NewCursorPosX;
            CursorPosY     = NewCursorPosY;
            StartOperation = true;
        }
        else
        {
            constexpr float TickRate = g_FrameRate / 25000.f;
            glm::mat4 Matrix         = EngineCore::Get().GetCameraMatrix();

            double const OffsetX = NewCursorPosX - CursorPosX;
            double const OffsetY = NewCursorPosY - CursorPosY;

            if (OffsetX != 0.F)
            {
                Matrix = glm::rotate(Matrix, static_cast<float>(OffsetX * TickRate), glm::vec3(0.0f, 0.0f, 1.0f));
            }

            if (OffsetY != 0.F)
            {
                Matrix = glm::rotate(Matrix, static_cast<float>(OffsetY * TickRate), glm::vec3(0.0f, 1.0f, 0.0f));
            }

            CursorPosX = NewCursorPosX;
            CursorPosY = NewCursorPosY;

            EngineCore::Get().SetCameraMatrix(Matrix);
        }
    }
    else if (ButtonEvent == GLFW_RELEASE)
    {
        CursorPosX     = 0.0;
        CursorPosY     = 0.0;
        StartOperation = false;
    }
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

    EngineCore::Get().Shutdown();
}

bool Window::IsInitialized() const
{
    return IsOpen() && EngineCore::Get().IsInitialized();
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

                    EngineCore::Get().DrawFrame(s_Window);
                    break;
                }
                case ApplicationEventFlags::LOAD_SCENE: {
                    EngineCore::Get().LoadScene(DEBUG_MODEL_OBJ, DEBUG_MODEL_TEX);
                    break;
                }
                case ApplicationEventFlags::UNLOAD_SCENE: {
                    EngineCore::Get().UnloadScene();
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
    ImGui::Begin("Hello World!");
    ImGui::Text("How r u doing?");
    ImGui::End();
}

bool Window::InitializeGLFW(std::uint16_t const Width, std::uint16_t const Height, std::string_view const Title)
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
    glfwSetErrorCallback(&GLFWErrorCallback);

    return s_Window != nullptr;
}

bool Window::InitializeEngineCore() const
{
    EngineCore::Get().Initialize(s_Window);

    if (DeviceManager::Get().UpdateDeviceProperties(s_Window))
    {
        return EngineCore::Get().IsInitialized();
    }

    return false;
}

bool Window::InitializeImGui() const
{
    try
    {
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGuiIO& IO                  = ImGui::GetIO();
        VkExtent2D const DisplaySize = Helpers::GetWindowExtent(
                s_Window,
                DeviceManager::Get().GetAvailablePhysicalDeviceSurfaceCapabilities());

        IO.DisplaySize = ImVec2(static_cast<float>(DisplaySize.width), static_cast<float>(DisplaySize.height));
    }
    catch (std::exception const& Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
        return false;
    }

    return true;
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