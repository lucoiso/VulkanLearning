// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <glm/ext.hpp>

module RenderCore.Types.Camera;

import RenderCore.Utils.EnumHelpers;

using namespace RenderCore;

glm::vec3 RenderCore::Camera::GetPosition() const
{
    return m_CameraPosition;
}

void RenderCore::Camera::SetPosition(glm::vec3 const& Position)
{
    m_CameraPosition = Position;
}

glm::vec3 RenderCore::Camera::GetFront() const
{
    return m_CameraFront;
}

void RenderCore::Camera::SetFront(glm::vec3 const& Front)
{
    m_CameraFront = Front;
}

glm::vec3 RenderCore::Camera::GetUp() const
{
    return m_CameraUp;
}

void RenderCore::Camera::SetUp(glm::vec3 const& Up)
{
    m_CameraUp = Up;
}

float RenderCore::Camera::GetSpeed() const
{
    return m_CameraSpeed;
}

void RenderCore::Camera::SetSpeed(float const Speed)
{
    m_CameraSpeed = Speed;
}

float RenderCore::Camera::GetYaw() const
{
    return m_CameraYaw;
}

void RenderCore::Camera::SetYaw(float const Yaw)
{
    m_CameraYaw = Yaw;
}

float RenderCore::Camera::GetPitch() const
{
    return m_CameraPitch;
}

void RenderCore::Camera::SetPitch(float const Pitch)
{
    m_CameraPitch = Pitch;
}

float RenderCore::Camera::GetSensitivity() const
{
    return m_CameraSensitivity;
}

void RenderCore::Camera::SetSensitivity(float const Sensitivity)
{
    m_CameraSensitivity = Sensitivity;
}

glm::mat4 RenderCore::Camera::GetMatrix() const
{
    return glm::lookAt(m_CameraPosition, m_CameraPosition + m_CameraFront, m_CameraUp);
}

CameraMovementStateFlags RenderCore::Camera::GetCameraMovementStateFlags()
{
    return m_CameraMovementStateFlags;
}

void RenderCore::Camera::SetCameraMovementStateFlags(CameraMovementStateFlags const State)
{
    m_CameraMovementStateFlags = State;
}

void RenderCore::Camera::UpdateCameraMovement(GLFWwindow* const Window, float const DeltaTime)
{
    float const CameraSpeed     = GetSpeed();
    glm::vec3 const CameraFront = GetFront();
    glm::vec3 const CameraRight = glm::normalize(glm::cross(CameraFront, GetUp()));

    glm::vec3 NewPosition = GetPosition();

    if ((m_CameraMovementStateFlags & CameraMovementStateFlags::FORWARD) == CameraMovementStateFlags::FORWARD)
    {
        NewPosition += CameraSpeed * CameraFront * DeltaTime;
    }

    if ((m_CameraMovementStateFlags & CameraMovementStateFlags::BACKWARD) == CameraMovementStateFlags::BACKWARD)
    {
        NewPosition -= CameraSpeed * CameraFront * DeltaTime;
    }

    if ((m_CameraMovementStateFlags & CameraMovementStateFlags::LEFT) == CameraMovementStateFlags::LEFT)
    {
        NewPosition -= CameraSpeed * CameraRight * DeltaTime;
    }

    if ((m_CameraMovementStateFlags & CameraMovementStateFlags::RIGHT) == CameraMovementStateFlags::RIGHT)
    {
        NewPosition += CameraSpeed * CameraRight * DeltaTime;
    }

    if ((m_CameraMovementStateFlags & CameraMovementStateFlags::UP) == CameraMovementStateFlags::UP)
    {
        NewPosition += CameraSpeed * GetUp() * DeltaTime;
    }

    if ((m_CameraMovementStateFlags & CameraMovementStateFlags::DOWN) == CameraMovementStateFlags::DOWN)
    {
        NewPosition -= CameraSpeed * GetUp() * DeltaTime;
    }

    SetPosition(NewPosition);
}

RenderCore::Camera& RenderCore::GetViewportCamera()
{
    static Camera ViewportCamera;
    return ViewportCamera;
}