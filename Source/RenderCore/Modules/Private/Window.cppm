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
import Timer.ExecutionCounter;

GLFWwindow* g_Window {nullptr};

bool InitializeGLFW(std::uint16_t const Width, std::uint16_t const Height, std::string_view const& Title, bool const bHeadless)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    if (glfwInit() == 0)
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, bHeadless ? GLFW_FALSE : GLFW_TRUE);

    g_Window = glfwCreateWindow(Width, Height, std::data(Title), nullptr, nullptr);

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

AsyncOperation<bool> Window::Initialize(std::uint16_t const Width, std::uint16_t const Height, std::string_view const& Title, bool const bHeadless)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

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
            if (RenderCore::EngineCore::Get().GetNumObjects() == 0U)
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
                    EngineCore::Get().UnloadAllScenes();
                    if (!std::empty(s_ModelPath))
                    {
                        [[maybe_unused]] auto const _ = EngineCore::Get().LoadScene(s_ModelPath);
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
            auto const& Objects = EngineCore::Get().GetObjects();
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