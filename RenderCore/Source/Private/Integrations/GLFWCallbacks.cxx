// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <GLFW/glfw3.h>
#include <boost/log/trivial.hpp>
#include <glm/ext.hpp>
#include <imgui.h>

module RenderCore.Integrations.GLFWCallbacks;

import RenderCore.Renderer;
import RenderCore.Runtime.Device;
import RenderCore.Types.Camera;
import RenderCore.Types.Transform;
import RenderCore.Types.RendererStateFlags;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.EnumHelpers;

using namespace RenderCore;

bool g_CanMovementCamera = false;

void RenderCore::GLFWWindowCloseRequested(GLFWwindow *const Window)
{
    glfwSetWindowShouldClose(Window, GLFW_TRUE);
}

void RenderCore::GLFWWindowResized([[maybe_unused]] GLFWwindow *const  Window,
                                   [[maybe_unused]] std::int32_t const Width,
                                   [[maybe_unused]] std::int32_t const Height)
{
    if (Width <= 0 || Height <= 0)
    {
        Renderer::AddStateFlag(RendererStateFlags::INVALID_SIZE);
        Renderer::AddStateFlag(RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE);
    }
    else
    {
        Renderer::RemoveStateFlag(RendererStateFlags::INVALID_SIZE);
        Renderer::RemoveStateFlag(RendererStateFlags::PENDING_DEVICE_PROPERTIES_UPDATE);
    }
}

void RenderCore::GLFWErrorCallback(std::int32_t const Error, char const *const Description)
{
    BOOST_LOG_TRIVIAL(error) << "[" << __func__ << "]: GLFW Error: " << Error << " - " << Description;
}

void RenderCore::GLFWKeyCallback([[maybe_unused]] GLFWwindow *const  Window,
                                 std::int32_t const                  Key,
                                 [[maybe_unused]] std::int32_t const Scancode,
                                 std::int32_t const                  Action,
                                 [[maybe_unused]] std::int32_t const Mods)
{
    Camera &                 Camera               = Renderer::GetMutableCamera();
    CameraMovementStateFlags CurrentMovementState = Camera.GetCameraMovementStateFlags();

    if (!g_CanMovementCamera)
    {
        Camera.SetCameraMovementStateFlags(CameraMovementStateFlags::NONE);
        return;
    }

    if (Action == GLFW_PRESS)
    {
        switch (Key)
        {
            case GLFW_KEY_W:
                AddFlags(CurrentMovementState, CameraMovementStateFlags::FORWARD);
                break;
            case GLFW_KEY_S:
                AddFlags(CurrentMovementState, CameraMovementStateFlags::BACKWARD);
                break;
            case GLFW_KEY_A:
                AddFlags(CurrentMovementState, CameraMovementStateFlags::LEFT);
                break;
            case GLFW_KEY_D:
                AddFlags(CurrentMovementState, CameraMovementStateFlags::RIGHT);
                break;
            case GLFW_KEY_Q:
                AddFlags(CurrentMovementState, CameraMovementStateFlags::DOWN);
                break;
            case GLFW_KEY_E:
            case GLFW_KEY_SPACE:
                AddFlags(CurrentMovementState, CameraMovementStateFlags::UP);
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
                RemoveFlags(CurrentMovementState, CameraMovementStateFlags::FORWARD);
                break;
            case GLFW_KEY_S:
                RemoveFlags(CurrentMovementState, CameraMovementStateFlags::BACKWARD);
                break;
            case GLFW_KEY_A:
                RemoveFlags(CurrentMovementState, CameraMovementStateFlags::LEFT);
                break;
            case GLFW_KEY_D:
                RemoveFlags(CurrentMovementState, CameraMovementStateFlags::RIGHT);
                break;
            case GLFW_KEY_Q:
                RemoveFlags(CurrentMovementState, CameraMovementStateFlags::DOWN);
                break;
            case GLFW_KEY_E:
            case GLFW_KEY_SPACE:
                RemoveFlags(CurrentMovementState, CameraMovementStateFlags::UP);
                break;
            default:
                break;
        }
    }

    Camera.SetCameraMovementStateFlags(CurrentMovementState);
}

void RenderCore::GLFWCursorPositionCallback(GLFWwindow *const Window, double const NewCursorPosX, double const NewCursorPosY)
{
    static double LastCursorPosX = NewCursorPosX;
    static double LastCursorPosY = NewCursorPosY;

    g_CanMovementCamera = glfwGetMouseButton(Window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_RELEASE;

    if (g_CanMovementCamera)
    {
        glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        Camera &Camera { Renderer::GetMutableCamera() };

        float const Sensitivity { Camera.GetSensitivity() * 0.1F };
        float const OffsetX { static_cast<float>(LastCursorPosX - NewCursorPosX) * Sensitivity };
        float const OffsetY { static_cast<float>(LastCursorPosY - NewCursorPosY) * Sensitivity };

        glm::vec3 Rotation { Camera.GetRotation() };

        Rotation.x -= OffsetX;
        Rotation.y += OffsetY;

        if (Rotation.y > 89.F)
        {
            Rotation.y = 89.F;
        }
        else if (Rotation.y < -89.F)
        {
            Rotation.y = -89.F;
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

void RenderCore::GLFWCursorScrollCallback([[maybe_unused]] GLFWwindow *const Window, [[maybe_unused]] double const OffsetX, double const OffsetY)
{
    Camera &    Camera = Renderer::GetMutableCamera();
    float const Zoom   = static_cast<float>(OffsetY) * 0.1f;
    Camera.SetPosition(Camera.GetPosition() + Camera.GetFront() * Zoom);
}

void RenderCore::InstallGLFWCallbacks(GLFWwindow *const Window, bool const InstallClose)
{
    if (InstallClose)
    {
        glfwSetWindowCloseCallback(Window, &GLFWWindowCloseRequested);
    }

    glfwSetWindowSizeCallback(Window, &GLFWWindowResized);
    glfwSetKeyCallback(Window, &GLFWKeyCallback);
    glfwSetCursorPosCallback(Window, &GLFWCursorPositionCallback);
    glfwSetScrollCallback(Window, &GLFWCursorScrollCallback);
}
