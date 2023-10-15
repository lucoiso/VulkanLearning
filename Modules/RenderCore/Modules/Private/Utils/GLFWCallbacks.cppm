// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <boost/log/trivial.hpp>
#include <glm/ext.hpp>
#include <imgui.h>

module RenderCore.Utils.GLFWCallbacks;

import RenderCore.EngineCore;
import RenderCore.Management.DeviceManagement;
import RenderCore.Types.Camera;
import RenderCore.Utils.Constants;
import RenderCore.Utils.EnumHelpers;

using namespace RenderCore;

bool g_CanMovementCamera = false;

void RenderCore::GLFWWindowCloseRequested(GLFWwindow* const Window)
{
    ShutdownEngine();
    glfwSetWindowShouldClose(Window, GLFW_TRUE);
}

void RenderCore::GLFWWindowResized(GLFWwindow* const Window, [[maybe_unused]] std::int32_t const Width, [[maybe_unused]] std::int32_t const Height)
{
    UpdateDeviceProperties(Window);
}

void RenderCore::GLFWErrorCallback(std::int32_t const Error, char const* const Description)
{
    BOOST_LOG_TRIVIAL(error) << "[" << __func__ << "]: GLFW Error: " << Error << " - " << Description;
}

void RenderCore::GLFWKeyCallback(GLFWwindow* const Window, std::int32_t const Key, [[maybe_unused]] std::int32_t const Scancode, std::int32_t const Action, [[maybe_unused]] std::int32_t const Mods)
{
    Camera& Camera                                = GetViewportCamera();
    CameraMovementStateFlags CurrentMovementState = Camera.GetCameraMovementStateFlags();

    if (Action == GLFW_PRESS)
    {
        switch (Key)
        {
            case GLFW_KEY_W:
                CurrentMovementState |= CameraMovementStateFlags::FORWARD;
                break;
            case GLFW_KEY_S:
                CurrentMovementState |= CameraMovementStateFlags::BACKWARD;
                break;
            case GLFW_KEY_A:
                CurrentMovementState |= CameraMovementStateFlags::LEFT;
                break;
            case GLFW_KEY_D:
                CurrentMovementState |= CameraMovementStateFlags::RIGHT;
                break;
            case GLFW_KEY_Q:
                CurrentMovementState |= CameraMovementStateFlags::DOWN;
                break;
            case GLFW_KEY_E:
            case GLFW_KEY_SPACE:
                CurrentMovementState |= CameraMovementStateFlags::UP;
                break;
            default:
                break;
        }
    }
    else if (Action == GLFW_RELEASE)
    {
        switch (Key)
        {
            case GLFW_KEY_W:
                CurrentMovementState &= ~CameraMovementStateFlags::FORWARD;
                break;
            case GLFW_KEY_S:
                CurrentMovementState &= ~CameraMovementStateFlags::BACKWARD;
                break;
            case GLFW_KEY_A:
                CurrentMovementState &= ~CameraMovementStateFlags::LEFT;
                break;
            case GLFW_KEY_D:
                CurrentMovementState &= ~CameraMovementStateFlags::RIGHT;
                break;
            case GLFW_KEY_Q:
                CurrentMovementState &= ~CameraMovementStateFlags::DOWN;
                break;
            case GLFW_KEY_E:
            case GLFW_KEY_SPACE:
                CurrentMovementState &= ~CameraMovementStateFlags::UP;
                break;
            default:
                break;
        }
    }

    Camera.SetCameraMovementStateFlags(CurrentMovementState);
}

void RenderCore::GLFWCursorPositionCallback(GLFWwindow* const Window, double const NewCursorPosX, double const NewCursorPosY)
{
    static double LastCursorPosX = NewCursorPosX;
    static double LastCursorPosY = NewCursorPosY;

    g_CanMovementCamera = glfwGetMouseButton(Window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_RELEASE && !ImGui::GetIO().WantCaptureMouse;

    if (g_CanMovementCamera)
    {
        glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        Camera& Camera {GetViewportCamera()};

        float const Sensitivity {Camera.GetSensitivity()};

        float const OffsetX {static_cast<float>(NewCursorPosX - LastCursorPosX) * Sensitivity};
        float const OffsetY {static_cast<float>(LastCursorPosY - NewCursorPosY) * Sensitivity};

        Rotator Rotation {Camera.GetRotation()};
        Rotation.Pitch += OffsetY;
        Rotation.Yaw += OffsetX;

        if (Rotation.Pitch > 89.F)
        {
            Rotation.Pitch = 89.F;
        }
        else if (Rotation.Pitch < -89.F)
        {
            Rotation.Pitch = -89.F;
        }

        Camera.SetRotation(Rotation);
    }
    else
    {
        glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    LastCursorPosX = NewCursorPosX;
    LastCursorPosY = NewCursorPosY;
}

void RenderCore::GLFWCursorScrollCallback([[maybe_unused]] GLFWwindow* const Window, [[maybe_unused]] double const OffsetX, double const OffsetY)
{
    Camera& Camera   = GetViewportCamera();
    float const Zoom = static_cast<float>(OffsetY) * 0.1f;
    Camera.SetPosition(Camera.GetPosition() + Camera.GetRotation().GetFront() * Zoom);
}
