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

using namespace RenderCore;

import <semaphore>;

import Timer.Manager;
import RenderCore.EngineCore;
import RenderCore.Management.DeviceManagement;
import RenderCore.Management.ImGuiManagement;
import RenderCore.Types.Camera;
import RenderCore.Utils.GLFWCallbacks;
import RenderCore.Utils.RenderUtils;
import RenderCore.Utils.Constants;
import RenderCore.Utils.Helpers;

GLFWwindow* g_Window {nullptr};

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

Window::Window()
    : m_RenderTimerManager(std::make_unique<Timer::Manager>())
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

        if (IsOpen())
        {
            glfwDestroyWindow(g_Window);
            glfwTerminate();
        }
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
            m_RenderTimerManager->SetTimer(
                    0U,
                    [this]() {
                        [[maybe_unused]] auto const _ = LoadObject(DEBUG_MODEL_OBJ, DEBUG_MODEL_TEX);
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

    std::lock_guard<std::mutex> Lock(g_CriticalEventMutex);
    std::binary_semaphore Semaphore {1U};

    m_RenderTimerManager->SetTimer(
            0U,
            [&Semaphore]() {
                ShutdownEngine();
                Semaphore.release();
            });

    Semaphore.acquire();
}

bool Window::IsInitialized()
{
    return IsEngineInitialized();
}

bool Window::IsOpen()
{
    return g_Window != nullptr && glfwWindowShouldClose(g_Window) == 0;
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
    }
    catch (std::exception const& Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
    }
}

void Window::CreateOverlay()
{
    ImGui::Begin("Vulkan Renderer Options", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    {
        ImGui::BeginGroup();
        {
            ImGui::Text("Frame Rate: %.1f", ImGui::GetIO().Framerate);
            ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
            ImGui::Text("Camera Position: %.3f, %.3f, %.3f", GetViewportCamera().GetPosition().X, GetViewportCamera().GetPosition().X, GetViewportCamera().GetPosition().Z);
            ImGui::Text("Camera Yaw: %.3f", GetViewportCamera().GetRotation().Yaw);
            ImGui::Text("Camera Pitch: %.3f", GetViewportCamera().GetRotation().Pitch);
            ImGui::Text("Camera Movement State: %d", static_cast<std::underlying_type_t<CameraMovementStateFlags>>(GetViewportCamera().GetCameraMovementStateFlags()));
        }
        ImGui::EndGroup();

        ImGui::Spacing();

        ImGui::BeginGroup();
        {
            constexpr size_t const MaxPathSize = 256;

            static std::string s_ModelPath(MaxPathSize, '\0');
            ImGui::InputText("Model Path", s_ModelPath.data(), MaxPathSize);

            static std::string s_TexturePath(MaxPathSize, '\0');
            ImGui::InputText("Texture Path", s_TexturePath.data(), MaxPathSize);

            if (ImGui::Button("Load Model"))
            {
                try
                {
                    UnloadObject(GetLoadedIDs());

                    std::string const ModelPathInternal   = s_ModelPath.substr(0, s_ModelPath.find('\0'));
                    std::string const TexturePathInternal = s_TexturePath.substr(0, s_TexturePath.find('\0'));

                    [[maybe_unused]] auto const _ = LoadObject(ModelPathInternal, TexturePathInternal);
                }
                catch (std::exception const& Ex)
                {
                    BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
                }
            }
        }
        ImGui::EndGroup();
    }
    ImGui::End();
}

void Window::RequestRender()
{
    std::lock_guard<std::mutex> Lock(g_CriticalEventMutex);

    if (IsOpen())
    {
        static double DeltaTime = glfwGetTime();
        DeltaTime               = glfwGetTime() - DeltaTime;

        GetViewportCamera().UpdateCameraMovement(static_cast<float>(DeltaTime));

        DrawImGuiFrame([this]() {
            CreateOverlay();
        });

        DrawFrame(g_Window);

        m_RenderTimerManager->SetTimer(
                1000U / g_FrameRate,
                [this]() {
                    RequestRender();
                });
    }
}