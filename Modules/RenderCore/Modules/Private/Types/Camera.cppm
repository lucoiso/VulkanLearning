// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <glm/ext.hpp>

module RenderCore.Types.Camera;

import RenderCore.Utils.EnumHelpers;

using namespace RenderCore;

Vector RenderCore::Camera::GetPosition() const
{
    return m_CameraPosition;
}

void RenderCore::Camera::SetPosition(Vector const& Position)
{
    m_CameraPosition = Position;
}

Rotator RenderCore::Camera::GetRotation() const
{
    return m_CameraRotation;
}

void RenderCore::Camera::SetRotation(Rotator const& Rotation)
{
    m_CameraRotation = Rotation;
}

float RenderCore::Camera::GetSpeed() const
{
    return m_CameraSpeed;
}

void RenderCore::Camera::SetSpeed(float const Speed)
{
    m_CameraSpeed = Speed;
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
    return glm::lookAt(m_CameraPosition.ToGlmVec3(), (m_CameraPosition + m_CameraRotation.GetFront()).ToGlmVec3(), m_CameraRotation.GetUp().ToGlmVec3());
}

CameraMovementStateFlags RenderCore::Camera::GetCameraMovementStateFlags()
{
    return m_CameraMovementStateFlags;
}

void RenderCore::Camera::SetCameraMovementStateFlags(CameraMovementStateFlags const State)
{
    m_CameraMovementStateFlags = State;
}

void RenderCore::Camera::UpdateCameraMovement(float const DeltaTime)
{
    float const CameraSpeed {GetSpeed()};
    Vector const CameraFront {m_CameraRotation.GetFront()};
    Vector const CameraRight {m_CameraRotation.GetRight()};

    Vector NewPosition {GetPosition()};

    if (HasFlag(m_CameraMovementStateFlags, CameraMovementStateFlags::FORWARD))
    {
        NewPosition += CameraSpeed * CameraFront * DeltaTime;
    }

    if (HasFlag(m_CameraMovementStateFlags, CameraMovementStateFlags::BACKWARD))
    {
        NewPosition -= CameraSpeed * CameraFront * DeltaTime;
    }

    if (HasFlag(m_CameraMovementStateFlags, CameraMovementStateFlags::LEFT))
    {
        NewPosition += CameraSpeed * CameraRight * DeltaTime;
    }

    if (HasFlag(m_CameraMovementStateFlags, CameraMovementStateFlags::RIGHT))
    {
        NewPosition -= CameraSpeed * CameraRight * DeltaTime;
    }

    if (HasFlag(m_CameraMovementStateFlags, CameraMovementStateFlags::UP))
    {
        NewPosition += CameraSpeed * m_CameraRotation.GetUp() * DeltaTime;
    }

    if (HasFlag(m_CameraMovementStateFlags, CameraMovementStateFlags::DOWN))
    {
        NewPosition -= CameraSpeed * m_CameraRotation.GetUp() * DeltaTime;
    }

    SetPosition(NewPosition);
}

RenderCore::Camera& RenderCore::GetViewportCamera()
{
    static Camera ViewportCamera;
    return ViewportCamera;
}