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

import RenderCore.Renderer;
import RenderCore.Management.DeviceManagement;
import RenderCore.Management.ImGuiManagement;
import RenderCore.Types.Camera;
import RenderCore.Input.GLFWCallbacks;
import RenderCore.Utils.Constants;
import RenderCore.Utils.Helpers;
import Timer.ExecutionCounter;

static bool g_HasWindow {false};

class Window::WindowImpl final
{
    GLFWwindow* m_Window {nullptr};

public:
    [[nodiscard]] bool Initialize(std::uint16_t const Width, std::uint16_t const Height, std::string_view const& Title, bool bMaximized, bool const bHeadless)
    {
        Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

        if (glfwInit() == 0)
        {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_MAXIMIZED, bMaximized ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_VISIBLE, bHeadless ? GLFW_FALSE : GLFW_TRUE);

        m_Window = glfwCreateWindow(Width, Height, std::data(Title), nullptr, nullptr);

        if (m_Window == nullptr)
        {
            throw std::runtime_error("Failed to create GLFW Window");
        }

        glfwSetWindowCloseCallback(m_Window, &GLFWWindowCloseRequested);
        glfwSetWindowSizeCallback(m_Window, &GLFWWindowResized);
        glfwSetKeyCallback(m_Window, &GLFWKeyCallback);
        glfwSetCursorPosCallback(m_Window, &GLFWCursorPositionCallback);
        glfwSetScrollCallback(m_Window, &GLFWCursorScrollCallback);

        return m_Window != nullptr;
    }

    void Shutdown()
    {
        Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

        glfwSetWindowShouldClose(m_Window, GLFW_TRUE);
        glfwDestroyWindow(m_Window);
        glfwTerminate();
        m_Window = nullptr;
    }

    [[nodiscard]] GLFWwindow* GetWindow() const
    {
        return m_Window;
    }

    [[nodiscard]] bool IsOpen() const
    {
        return m_Window && !glfwWindowShouldClose(m_Window);
    }
};

Window::Window()
    : m_WindowImpl(std::make_unique<WindowImpl>())
{
    if (g_HasWindow)
    {
        throw std::runtime_error("Only one window can be created at a time");
    }

    g_HasWindow = true;
}

Window::~Window()
{
    g_HasWindow = false;
    
    try
    {
        Shutdown().Get();
    }
    catch (...)
    {
    }
}

AsyncOperation<bool> Window::Initialize(std::uint16_t const Width, std::uint16_t const Height, std::string_view const& Title, bool bMaximized, bool const bHeadless)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    if (IsInitialized())
    {
        co_return false;
    }

    try
    {
        if (m_WindowImpl->Initialize(Width, Height, Title, bMaximized, bHeadless) && m_Renderer.Initialize(m_WindowImpl->GetWindow()))
        {
            std::binary_semaphore Semaphore {0U};
            {
                Renderer::GetRenderTimerManager().SetTimer(
                        0U,
                        [this, &Semaphore] {
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
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    if (IsInitialized())
    {
        std::binary_semaphore Semaphore {0U};
        {
            Renderer::GetRenderTimerManager().SetTimer(
                    0U,
                    [&Semaphore, this] {
                        m_Renderer.Shutdown();
                        Semaphore.release();
                    });
        }
        Semaphore.acquire();
    }

    if (IsOpen())
    {
        m_WindowImpl->Shutdown();
    }

    co_return;
}

bool Window::IsInitialized() const
{
    return m_WindowImpl && m_Renderer.IsInitialized();
}

bool Window::IsOpen() const
{
    return m_WindowImpl && m_WindowImpl->IsOpen();
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
            ImGui::Text("Frame Rate: %.2f", ImGui::GetIO().Framerate);
            ImGui::Text("Frame Time: %.2f ms", 1000.0f / ImGui::GetIO().Framerate);
            ImGui::Text("Camera Position: %.2f, %.2f, %.2f", GetViewportCamera().GetPosition().X, GetViewportCamera().GetPosition().X, GetViewportCamera().GetPosition().Z);
            ImGui::Text("Camera Yaw: %.2f", GetViewportCamera().GetRotation().Yaw);
            ImGui::Text("Camera Pitch: %.2f", GetViewportCamera().GetRotation().Pitch);
            ImGui::Text("Camera Movement State: %d", static_cast<std::underlying_type_t<CameraMovementStateFlags>>(GetViewportCamera().GetCameraMovementStateFlags()));

            float CameraSpeed = GetViewportCamera().GetSpeed();
            ImGui::InputFloat("Camera Speed", &CameraSpeed, 0.1F, 1.0F, "%.2f");
            GetViewportCamera().SetSpeed(CameraSpeed);
        }
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            constexpr const char* OptionNone = "None";
            static std::string SelectedItem  = OptionNone;
            if (m_Renderer.GetNumObjects() == 0U)
            {
                SelectedItem = OptionNone;
            }

            if (ImGui::BeginCombo("glTF Scene", std::data(SelectedItem)))
            {
                static std::unordered_map<std::string, std::string> const OptionsMap = GetAvailableglTFAssetsInDirectory("Resources/Assets", {".gltf", ".glb"});
                static std::string s_ModelPath                                       = OptionsMap.at(SelectedItem);
                bool SelectionChanged                                                = false;

                for (auto const& [Name, Path]: OptionsMap)
                {
                    bool const bIsSelected = SelectedItem == Name;
                    if (ImGui::Selectable(std::data(Name), bIsSelected))
                    {
                        SelectedItem     = Name;
                        s_ModelPath      = Path;
                        SelectionChanged = true;
                    }

                    if (bIsSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();

                if (SelectionChanged)
                {
                    m_Renderer.UnloadAllScenes();
                    if (!std::empty(s_ModelPath))
                    {
                        [[maybe_unused]] auto const _ = m_Renderer.LoadScene(s_ModelPath);
                    }
                }
            }
        }
        ImGui::EndGroup();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::BeginGroup();
        {
            auto const& Objects = m_Renderer.GetObjects();
            ImGui::Text("Loaded Objects: %d", std::size(Objects));
            ImGui::Spacing();

            for (auto const& Object: Objects)
            {
                if (!Object)
                {
                    continue;
                }

                ImGui::Text("Name: %s", std::data(Object->GetName()));
                ImGui::SameLine();
                ImGui::Text("ID: %d", Object->GetID());

                ImGui::Spacing();

                float Position[3] = {Object->GetPosition().X, Object->GetPosition().Y, Object->GetPosition().Z};
                ImGui::InputFloat3(std::format("{} Position", Object->GetName()).c_str(), &Position[0], "%.2f");
                Object->SetPosition({Position[0], Position[1], Position[2]});

                float Scale[3] = {Object->GetScale().X, Object->GetScale().Y, Object->GetScale().Z};
                ImGui::InputFloat3(std::format("{} Scale", Object->GetName()).c_str(), &Scale[0], "%.2f");
                Object->SetScale({Scale[0], Scale[1], Scale[2]});

                float Rotation[3] = {Object->GetRotation().Pitch, Object->GetRotation().Yaw, Object->GetRotation().Roll};
                ImGui::InputFloat3(std::format("{} Rotation", Object->GetName()).c_str(), &Rotation[0], "%.2f");
                Object->SetRotation({Rotation[0], Rotation[1], Rotation[2]});

                if (ImGui::Button(std::format("Destroy {}", Object->GetName()).c_str()))
                {
                    Object->Destroy();
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
        GetViewportCamera().UpdateCameraMovement(static_cast<float>(m_Renderer.GetDeltaTime()));

        DrawImGuiFrame([this] {
            CreateOverlay();
        });

        m_Renderer.DrawFrame(m_WindowImpl->GetWindow());

        Renderer::GetRenderTimerManager().SetTimer(
                1000U / g_FrameRate,
                [this]() {
                    RequestRender();
                });
    }
}