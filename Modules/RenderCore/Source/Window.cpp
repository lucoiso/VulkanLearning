// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

#include "Window.h"
#include "VulkanRenderCore.h"
#include "Managers/VulkanDeviceManager.h"
#include "Utils/VulkanConstants.h"
#include "Utils/RenderCoreHelpers.h"
#include "Utils/GLFWCallbacks.h"
#include "Types/ApplicationEventFlags.h"
#include <Timer/Manager.h>
#include <stdexcept>
#include <boost/log/trivial.hpp>
#include <GLFW/glfw3.h>

using namespace RenderCore;

class Window::Impl final // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
    Impl(Impl const&)            = delete;
    Impl& operator=(Impl const&) = delete;

    Impl()
        : m_MainThreadID(std::this_thread::get_id())
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

    bool Initialize(std::uint16_t const Width, std::uint16_t const Height, std::string_view const Title)
    {
        if (IsInitialized())
        {
            return false;
        }

        try
        {
            if (InitializeGLFW(Width, Height, Title) && InitializeVulkanRenderCore())
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

    void Shutdown()
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

        VulkanRenderCore::Get().Shutdown();
    }

    [[nodiscard]] bool IsInitialized() const
    {
        return IsOpen() && VulkanRenderCore::Get().IsInitialized();
    }

    [[nodiscard]] bool IsOpen() const
    {
        return m_Window != nullptr && glfwWindowShouldClose(m_Window) == 0;
    }

    void ProcessEvents()
    {
        if (!IsInitialized() || m_MainThreadID != std::this_thread::get_id())
        {
            return;
        }

        try
        {
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

                switch (EventFlags) // NOLINT(clang-diagnostic-switch-enum)
                {
                    case ApplicationEventFlags::DRAW_FRAME:
                        {
                            if (ProcessedEvents[EventFlags] > 0U)
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
        catch (std::exception const& Ex)
        {
            BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
        }
    }

private:
    bool InitializeGLFW(std::uint16_t const Width, std::uint16_t const Height, std::string_view const Title)
    {
        if (glfwInit() == 0)
        {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        if (m_Window = glfwCreateWindow(Width, Height, Title.data(), nullptr, nullptr);
            m_Window == nullptr)
        {
            throw std::runtime_error("Failed to create GLFW Window");
        }

        glfwSetWindowCloseCallback(m_Window, &GLFWWindowCloseRequested);
        glfwSetWindowSizeCallback(m_Window, &GLFWWindowResized);
        glfwSetKeyCallback(m_Window, &GLFWKeyCallback);
        glfwSetErrorCallback(&GLFWErrorCallback);

        return m_Window != nullptr;
    }

    [[nodiscard]] bool InitializeVulkanRenderCore() const
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
        Timer::Manager::Get().SetTickInterval(std::chrono::milliseconds(1));

        {
            // Draw Frame
            constexpr Timer::Parameters DrawFrameTimerParameters{
                .EventID = static_cast<std::uint8_t>(ApplicationEventFlags::DRAW_FRAME),
                .Interval = 1000U / (g_FrameRate > 0 ? g_FrameRate : 1U),
                .RepeatCount = std::nullopt
            };

            m_DrawTimerID = Timer::Manager::Get().StartTimer(DrawFrameTimerParameters, m_EventIDQueue);
        }

        {
            // Load Scene: Testing Only
            constexpr Timer::Parameters LoadSceneTimerParameters{
                .EventID = static_cast<std::uint8_t>(ApplicationEventFlags::LOAD_SCENE),
                .Interval = 3000U,
                .RepeatCount = 0
            };

            Timer::Manager::Get().StartTimer(LoadSceneTimerParameters, m_EventIDQueue);
        }

        {
            // Unload Scene: Testing Only
            constexpr Timer::Parameters UnLoadSceneTimerParameters{
                .EventID = static_cast<std::uint8_t>(ApplicationEventFlags::UNLOAD_SCENE),
                .Interval = 5000U,
                .RepeatCount = 0U
            };

            Timer::Manager::Get().StartTimer(UnLoadSceneTimerParameters, m_EventIDQueue);
        }
    }

    GLFWwindow* m_Window{
        nullptr
    };
    std::uint64_t m_DrawTimerID{
        0U
    };
    std::queue<std::uint8_t> m_EventIDQueue;
    std::mutex m_Mutex;
    std::thread::id m_MainThreadID;
};

Window::Window()
    : m_Impl(std::make_unique<Impl>())
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

void Window::Shutdown() const
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

void Window::PollEvents() const
{
    try
    {
        glfwPollEvents();

        if (IsInitialized())
        {
            m_Impl->ProcessEvents();
        }
    }
    catch (std::exception const& Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
    }
}
