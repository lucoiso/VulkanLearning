// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <glm/ext.hpp>

module RenderCore.Types.Camera;

import RenderCore.Management.BufferManagement;
import RenderCore.Utils.EnumHelpers;
import RenderCore.Utils.Constants;

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

float RenderCore::Camera::GetFieldOfView() const
{
    return m_FieldOfView;
}

void RenderCore::Camera::SetFieldOfView(float const FieldOfView)
{
    m_FieldOfView = FieldOfView;
}

float RenderCore::Camera::GetNearPlane() const
{
    return m_NearPlane;
}

void RenderCore::Camera::SetNearPlane(float const NearPlane)
{
    m_NearPlane = NearPlane;
}

float RenderCore::Camera::GetFarPlane() const
{
    return m_FarPlane;
}

void RenderCore::Camera::SetFarPlane(float const FarPlane)
{
    m_FarPlane = FarPlane;
}

float RenderCore::Camera::GetDrawDistance() const
{
    return m_DrawDistance;
}

void RenderCore::Camera::SetDrawDistance(float const DrawDistance)
{
    m_DrawDistance = DrawDistance;
}

glm::mat4 RenderCore::Camera::GetViewMatrix() const
{
    return glm::lookAt(m_CameraPosition.ToGlmVec3(), (m_CameraPosition + m_CameraRotation.GetFront()).ToGlmVec3(), m_CameraRotation.GetUp().ToGlmVec3());
}

glm::mat4 RenderCore::Camera::GetProjectionMatrix() const
{
    auto const& [Width, Height] = GetSwapChainExtent();
    glm::mat4 Projection        = glm::perspective(glm::radians(m_FieldOfView), static_cast<float>(Width) / static_cast<float>(Height), m_NearPlane, m_FarPlane);
    Projection[1][1] *= -1;

    return Projection;
}

CameraMovementStateFlags RenderCore::Camera::GetCameraMovementStateFlags() const
{
    return m_CameraMovementStateFlags;
}

void RenderCore::Camera::SetCameraMovementStateFlags(CameraMovementStateFlags const State)
{
    m_CameraMovementStateFlags = State;
}

void RenderCore::Camera::UpdateCameraMovement(float const DeltaTime)
{
    float const CameraSpeed {GetSpeed() * 0.01F};
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

bool RenderCore::Camera::IsInsideCameraFrustum(Vector const& TestLocation) const
{
    glm::mat4 const ViewProjectionMatrix = GetProjectionMatrix() * GetViewMatrix();

    glm::vec4 const HomogeneousTestLocation = glm::vec4(TestLocation.ToGlmVec3(), 1.0f);
    glm::vec4 const ClipSpaceTestLocation   = ViewProjectionMatrix * HomogeneousTestLocation;

    return std::abs(ClipSpaceTestLocation.x) <= std::abs(ClipSpaceTestLocation.w) && std::abs(ClipSpaceTestLocation.y) <= std::abs(ClipSpaceTestLocation.w) && std::abs(ClipSpaceTestLocation.z) <= std::abs(ClipSpaceTestLocation.w);
}

bool RenderCore::Camera::IsInAllowedDistance(Vector const& TestLocation) const
{
    Vector const CameraToTestLocation  = TestLocation - GetPosition();
    float const DistanceToTestLocation = CameraToTestLocation.Length();

    return DistanceToTestLocation <= GetDrawDistance();
}

bool RenderCore::Camera::CanDrawObject(std::shared_ptr<Object> const& Object) const
{
    if (!Object)
    {
        return false;
    }

    if constexpr (g_EnableExperimentalFrustumCulling)
    {
        return GetViewportCamera().IsInsideCameraFrustum(Object->GetPosition()) && GetViewportCamera().IsInAllowedDistance(Object->GetPosition());
    }

    return true;
}