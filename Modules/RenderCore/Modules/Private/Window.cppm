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
import RenderCore.Utils.Constants;
import RenderCore.Utils.Helpers;

GLFWwindow* g_Window {nullptr};

bool InitializeGLFW(std::uint16_t const Width, std::uint16_t const Height, std::string_view const Title, bool const bHeadless)
{
    if (glfwInit() == 0)
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, bHeadless ? GLFW_FALSE : GLFW_TRUE);

    g_Window = glfwCreateWindow(Width, Height, Title.data(), nullptr, nullptr);

    if (g_Window == nullptr)
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

Window::Window()
    : m_RenderTimerManager(std::make_unique<Timer::Manager>())
{
}

Window::~Window()
{
    try
    {
        Shutdown().Get();
    }
    catch (...)
    {
    }
}

AsyncOperation<bool> Window::Initialize(std::uint16_t const Width, std::uint16_t const Height, std::string_view const Title, bool const bHeadless)
{
    if (IsInitialized())
    {
        co_return false;
    }

    try
    {
        if (InitializeGLFW(Width, Height, Title, bHeadless) && EngineCore::Get().InitializeEngine(g_Window))
        {
            std::binary_semaphore Semaphore {0U};
            {
                m_RenderTimerManager->SetTimer(
                        0U,
                        [this, &Semaphore]() {
                            [[maybe_unused]] auto const _ = EngineCore::Get().LoadScene(DEBUG_MODEL_OBJ, DEBUG_MODEL_TEX);
                            RequestRender();
                            Semaphore.release();
                        });
            }
            Semaphore.acquire();
            co_return true;
        }
    }
    catch (std::exception const& Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
        Shutdown().Get();
    }

    co_return false;
}

AsyncTask Window::Shutdown()
{
    if (IsInitialized())
    {
        std::binary_semaphore Semaphore {0U};
        {
            m_RenderTimerManager->SetTimer(
                    0U,
                    [&Semaphore]() {
                        EngineCore::Get().ShutdownEngine();
                        Semaphore.release();
                    });
        }
        Semaphore.acquire();
    }

    if (IsOpen())
    {
        glfwSetWindowShouldClose(g_Window, GLFW_TRUE);
        glfwDestroyWindow(g_Window);
        glfwTerminate();
        g_Window = nullptr;
    }

    co_return;
}

bool Window::IsInitialized()
{
    return EngineCore::Get().IsEngineInitialized();
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
        static double LastFrameTime = glfwGetTime();

        double const CurrentTime = glfwGetTime();
        {
            glfwPollEvents();
            EngineCore::Get().Tick(CurrentTime - LastFrameTime);
        }
        LastFrameTime = CurrentTime;
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
                    EngineCore::Get().UnloadAllScenes();

                    std::string const ModelPathInternal   = s_ModelPath.substr(0, s_ModelPath.find('\0'));
                    std::string const TexturePathInternal = s_TexturePath.substr(0, s_TexturePath.find('\0'));

                    [[maybe_unused]] auto const _ = EngineCore::Get().LoadScene(ModelPathInternal, TexturePathInternal);
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
    if (IsOpen())
    {
        static double DeltaTime = glfwGetTime();
        DeltaTime               = glfwGetTime() - DeltaTime;

        GetViewportCamera().UpdateCameraMovement(static_cast<float>(DeltaTime));

        DrawImGuiFrame([this]() {
            CreateOverlay();
        });

        EngineCore::Get().DrawFrame(g_Window);

        m_RenderTimerManager->SetTimer(
                1000U / g_FrameRate,
                [this]() {
                    RequestRender();
                });
    }
}