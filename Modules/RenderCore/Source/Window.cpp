// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "Window.h"
#include "VulkanRenderCore.h"
#include "Managers/VulkanDeviceManager.h"
#include "Utils/VulkanConstants.h"
#include "Utils/RenderCoreHelpers.h"
#include "Utils/GLFWCallbacks.h"
#include "Types/ApplicationEventFlags.h"
#include <Timer/TimerManager.h>
#include <stdexcept>
#include <boost/log/trivial.hpp>
#include <GLFW/glfw3.h>

using namespace RenderCore;

class Window::Impl
{
public:
    Impl(const Window::Impl &) = delete;
    Impl &operator=(const Window::Impl &) = delete;

    Impl()
        : m_Window(nullptr)
        , m_DrawTimerID(0u)
        , m_EventIDQueue()
        , m_MainThreadID(std::this_thread::get_id())
    {
    }

    ~Impl()
    {
        if (!IsInitialized())
        {
            return;
        }

        Shutdown();

        glfwDestroyWindow(m_Window);
        glfwTerminate();
    }

    bool Initialize(const std::uint16_t Width, const std::uint16_t Height, const std::string_view Title)
    {
        if (IsInitialized())
        {
            return false;
        }

        try
        {
            if (InitializeGLFW(Width, Height, Title) && InitializeVulkanRenderCore())
            {
                VulkanRenderCore::Get().LoadScene(DEBUG_MODEL_OBJ, DEBUG_MODEL_TEX);
                RegisterTimers();
            }
        }
        catch (const std::exception &Ex)
        {
            BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
            Shutdown();
        }

        return IsInitialized();
    }

    void Shutdown()
    {
        if (!IsInitialized())
        {
            return;
        }

        std::lock_guard<std::mutex> Lock(m_Mutex);

        Timer::TimerManager::Get().StopTimer(m_DrawTimerID);
        while (!m_EventIDQueue.empty())
        {
            m_EventIDQueue.pop();
        }

        VulkanRenderCore::Get().Shutdown();
    }

    bool IsInitialized() const
    {
        return IsOpen() && VulkanRenderCore::Get().IsInitialized();
    }

    bool IsOpen() const
    {
        return m_Window && !glfwWindowShouldClose(m_Window);
    }

    void ProcessEvents()
    {
        if (!IsInitialized() || m_MainThreadID != std::this_thread::get_id())
        {
            return;
        }

        try
        {
            std::lock_guard<std::mutex> Lock(m_Mutex);
            std::unordered_map<ApplicationEventFlags, std::uint8_t> ProcessedEvents;

            for (std::uint8_t Iterator = 0u; Iterator < std::underlying_type<ApplicationEventFlags>::type(ApplicationEventFlags::MAX); ++Iterator)
            {
                ProcessedEvents.emplace(static_cast<ApplicationEventFlags>(Iterator), 0u);
            }

            while (!m_EventIDQueue.empty())
            {
                const ApplicationEventFlags EventFlags = static_cast<ApplicationEventFlags>(m_EventIDQueue.front());
                m_EventIDQueue.pop();

                switch (EventFlags)
                {
                case ApplicationEventFlags::DRAW_FRAME:
                {
                    if (ProcessedEvents[EventFlags] > 0u)
                    {
                        break;
                    }

                    VulkanRenderCore::Get().DrawFrame(m_Window);
                    break;
                }
                case ApplicationEventFlags::LOAD_SCENE:
                {
                    VulkanRenderCore::Get().LoadScene(DEBUG_MODEL_OBJ, DEBUG_MODEL_TEX);
                    break;
                }
                case ApplicationEventFlags::UNLOAD_SCENE:
                {
                    VulkanRenderCore::Get().UnloadScene();
                    break;
                }
                default:
                    break;
                }

                ++ProcessedEvents[EventFlags];
            }
        }
        catch (const std::exception &Ex)
        {
            BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
        }
    }

private:

    bool InitializeGLFW(const std::uint16_t Width, const std::uint16_t Height, const std::string_view Title)
    {
        if (!glfwInit())
        {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        if (m_Window = glfwCreateWindow(Width, Height, Title.data(), nullptr, nullptr); !m_Window)
        {
            throw std::runtime_error("Failed to create GLFW Window");
        }

        glfwSetWindowCloseCallback(m_Window, &GLFWWindowCloseRequested);
        glfwSetWindowSizeCallback(m_Window, &GLFWWindowResized);
        glfwSetKeyCallback(m_Window, &GLFWKeyCallback);
        glfwSetErrorCallback(&GLFWErrorCallback);

        return m_Window != nullptr;
    }

    bool InitializeVulkanRenderCore()
    {
        VulkanRenderCore::Get().Initialize(m_Window);

        if (VulkanDeviceManager::Get().UpdateDeviceProperties(m_Window))
        {
            return VulkanRenderCore::Get().IsInitialized();
        }

        return false;
    }

    void RegisterTimers()
    {
        constexpr std::uint32_t DrawFrameTimerInterval = 1000u / (g_FrameRate > 0 ? g_FrameRate : 1u);
        Timer::TimerManager::Get().SetTickInterval(std::chrono::milliseconds(DrawFrameTimerInterval));

        { // Draw Frame
            const Timer::TimerParameters DrawFrameTimerParameters {
                .EventID = static_cast<std::uint8_t>(ApplicationEventFlags::DRAW_FRAME),
                .Interval = DrawFrameTimerInterval,
                .RepeatCount = std::nullopt};

            m_DrawTimerID = Timer::TimerManager::Get().StartTimer(DrawFrameTimerParameters, m_EventIDQueue);
        }

        {
            // Load Scene: Testing Only
            // constexpr std::uint32_t LoadSceneTimerDelayMs = 5000u;
            // Timer::TimerManager::Get().StartTimer(Timer::Tags::SingleTime, static_cast<std::uint8_t>(ApplicationEventFlags::LOAD_SCENE), LoadSceneTimerDelayMs, m_EventIDQueue);
        }

        { // Unload Scene: Testing Only
          // constexpr std::uint32_t UnloadSceneTimerDelayMs = 10000u;
          // Timer::TimerManager::Get().StartTimer(Timer::Tags::SingleTime, static_cast<std::uint8_t>(ApplicationEventFlags::UNLOAD_SCENE), UnloadSceneTimerDelayMs, m_EventIDQueue);
        }
    }

    GLFWwindow *m_Window;
    std::uint64_t m_DrawTimerID;
    std::queue<std::uint8_t> m_EventIDQueue;
    std::mutex m_Mutex;
    std::thread::id m_MainThreadID;
};

Window::Window()
    : m_Impl(std::make_unique<Window::Impl>())
{
}

Window::~Window()
{
    if (!IsInitialized())
    {
        return;
    }

    Shutdown();
}

bool Window::Initialize(const std::uint16_t Width, const std::uint16_t Height, const std::string_view Title)
{
    if (IsInitialized())
    {
        return false;
    }

    if (m_Impl->Initialize(Width, Height, Title))
    {
        CreateOverlay();
    }
    else
    {
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

    m_Impl->Shutdown();
}

bool Window::IsInitialized() const
{
    return m_Impl && m_Impl->IsInitialized();
}

bool Window::IsOpen() const
{
    return m_Impl && m_Impl->IsOpen();
}

void Window::PollEvents()
{
    try
    {
        glfwPollEvents();

        if (IsInitialized())
        {
            m_Impl->ProcessEvents();
        }
    }
    catch (const std::exception &Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
    }
}
