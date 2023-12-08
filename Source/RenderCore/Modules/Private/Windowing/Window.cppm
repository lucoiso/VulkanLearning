// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <boost/log/trivial.hpp>
#include <imgui.h>
#include <numeric>
#include <volk.h>

module RenderCore.Window;

using namespace RenderCore;

import <semaphore>;

import RenderCore.Renderer;
import RenderCore.Management.DeviceManagement;
import RenderCore.Management.ImGuiManagement;
import RenderCore.Types.Camera;
import RenderCore.Utils.Constants;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.EnumHelpers;
import Timer.ExecutionCounter;

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

AsyncOperation<bool> Window::Initialize(std::uint16_t const Width, std::uint16_t const Height, std::string_view const& Title, InitializationFlags const Flags)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    if (IsInitialized())
    {
        co_return false;
    }

    try
    {
        if (m_GLFWHandler.Initialize(Width, Height, Title, Flags) && m_Renderer.Initialize(m_GLFWHandler.GetWindow()))
        {
            std::binary_semaphore Semaphore {0U};
            {
                Renderer::GetRenderTimerManager().SetTimer(
                        std::chrono::milliseconds(0U),
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
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    if (IsInitialized())
    {
        std::binary_semaphore Semaphore {0U};
        {
            Renderer::GetRenderTimerManager().SetTimer(
                    std::chrono::milliseconds(0U),
                    [&Semaphore, this] {
                        m_Renderer.Shutdown(m_GLFWHandler.GetWindow());
                        Semaphore.release();
                    });
        }
        Semaphore.acquire();
    }

    if (IsOpen())
    {
        m_GLFWHandler.Shutdown();
    }

    co_return;
}

bool Window::IsInitialized() const
{
    return m_Renderer.IsInitialized();
}

bool Window::IsOpen() const
{
    return m_GLFWHandler.IsOpen();
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
    ImGui::Begin("Options", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    {
        constexpr auto OptionNone                                              = "None";
        static std::unordered_map<std::string, std::string> const s_OptionsMap = GetAvailableglTFAssetsInDirectory("Resources/Assets", {".gltf", ".glb"});
        static std::string s_SelectedItem                                      = OptionNone;
        static std::string s_ModelPath                                         = s_OptionsMap.at(s_SelectedItem);

        if (ImGui::CollapsingHeader("Status "))
        {
            ImGui::Text("Frame Rate: %.3fms", m_Renderer.GetFrameTime());
            ImGui::Text("Frame Time: %.0f FPS", 1.f / m_Renderer.GetFrameTime());

            ImGui::Text("Camera Position: %.2f, %.2f, %.2f", m_Renderer.GetCamera().GetPosition().X, m_Renderer.GetCamera().GetPosition().X, m_Renderer.GetCamera().GetPosition().Z);
            ImGui::Text("Camera Yaw: %.2f", m_Renderer.GetCamera().GetRotation().Yaw);
            ImGui::Text("Camera Pitch: %.2f", m_Renderer.GetCamera().GetRotation().Pitch);
            ImGui::Text("Camera Movement State: %d", static_cast<std::underlying_type_t<CameraMovementStateFlags>>(m_Renderer.GetCamera().GetCameraMovementStateFlags()));

            float CameraSpeed = m_Renderer.GetCamera().GetSpeed();
            ImGui::InputFloat("Camera Speed", &CameraSpeed, 0.1F, 1.F, "%.2f");
            m_Renderer.GetMutableCamera().SetSpeed(CameraSpeed);

            float CameraSensitivity = m_Renderer.GetCamera().GetSensitivity();
            ImGui::InputFloat("Camera Sensitivity", &CameraSensitivity, 0.1F, 1.F, "%.2f");
            m_Renderer.GetMutableCamera().SetSensitivity(CameraSensitivity);

            if (m_Renderer.GetNumObjects() == 0U)
            {
                s_SelectedItem = OptionNone;
            }

            if (ImGui::BeginCombo("glTF Scene", std::data(s_SelectedItem)))
            {
                bool SelectionChanged = false;

                for (auto const& [Name, Path]: s_OptionsMap)
                {
                    bool const bIsSelected = s_SelectedItem == Name;
                    if (ImGui::Selectable(std::data(Name), bIsSelected))
                    {
                        s_SelectedItem   = Name;
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

        if (auto const& Objects = m_Renderer.GetObjects();
            ImGui::CollapsingHeader(std::format("Loaded Objects: {} ", m_Renderer.GetNumObjects()).c_str()))
        {
            if (!std::empty(Objects))
            {
                ImGui::Text("Loaded Objects: %d", m_Renderer.GetNumObjects());
                ImGui::Text("Total Triangles: %d", std::accumulate(std::begin(Objects),
                                                                   std::end(Objects),
                                                                   0U,
                                                                   [](std::uint32_t const Sum, auto const& Object) {
                                                                       return Sum + Object->GetTrianglesCount();
                                                                   }));

                if (ImGui::Button("Reload Scene"))
                {
                    m_Renderer.UnloadAllScenes();
                    [[maybe_unused]] auto const _ = m_Renderer.LoadScene(s_ModelPath);
                }

                ImGui::SameLine();

                if (ImGui::Button("Destroy All"))
                {
                    m_Renderer.UnloadAllScenes();
                }
            }

            for (auto const& Object: Objects)
            {
                if (!Object)
                {
                    continue;
                }

                if (ImGui::CollapsingHeader(std::format("[{}] {}", Object->GetID(), std::data(Object->GetName())).c_str()))
                {
                    ImGui::Text("ID: %d", Object->GetID());
                    ImGui::Text("Name: %s", std::data(Object->GetName()));
                    ImGui::Text("Path: %s", std::data(Object->GetPath()));
                    ImGui::Text("Triangles Count: %d", Object->GetTrianglesCount());

                    ImGui::Separator();

                    ImGui::Text("Transform");
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
        }
    }
    ImGui::End();
}

void Window::RequestRender()
{
    if (IsInitialized() && IsOpen())
    {
        m_Renderer.GetMutableCamera().UpdateCameraMovement(static_cast<float>(m_Renderer.GetDeltaTime()));

        DrawImGuiFrame([this] {
            CreateOverlay();
        });

        m_Renderer.DrawFrame(m_GLFWHandler.GetWindow(), m_Renderer.GetCamera());

        Renderer::GetRenderTimerManager().SetTimer(
                std::chrono::milliseconds(1000U / (2U * g_FrameRate)),
                [this] {
                    RequestRender();
                });
    }
}