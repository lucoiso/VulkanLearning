// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <glm/ext.hpp>
#include <Volk/volk.h>

module RenderCore.Types.Camera;

import RenderCore.Runtime.Memory;
import RenderCore.Runtime.SwapChain;
import RenderCore.Utils.EnumHelpers;
import RenderCore.Utils.Constants;
import RenderCore.Types.UniformBufferObject;

using namespace RenderCore;

glm::vec3 Camera::GetPosition() const
{
    return m_Position;
}

void Camera::SetPosition(glm::vec3 const &Value)
{
    if (m_Position != Value)
    {
        m_Position      = Value;
        m_IsRenderDirty = true;
    }
}

glm::vec3 Camera::GetRotation() const
{
    return m_Rotation;
}

void Camera::SetRotation(glm::vec3 const &Value)
{
    if (m_Rotation != Value)
    {
        m_Rotation      = Value;
        m_IsRenderDirty = true;
    }
}

float Camera::GetSpeed() const
{
    return m_Speed;
}

void Camera::SetSpeed(float const Value)
{
    m_Speed = Value;
}

float Camera::GetSensitivity() const
{
    return m_Sensitivity;
}

void Camera::SetSensitivity(float const Value)
{
    m_Sensitivity = Value;
}

float Camera::GetFieldOfView() const
{
    return m_FieldOfView;
}

void Camera::SetFieldOfView(float const Value)
{
    if (m_FieldOfView != Value)
    {
        m_FieldOfView   = Value;
        m_IsRenderDirty = true;
    }
}

float Camera::GetNearPlane() const
{
    return m_NearPlane;
}

void Camera::SetNearPlane(float const Value)
{
    if (m_NearPlane != Value)
    {
        m_NearPlane     = Value;
        m_IsRenderDirty = true;
    }
}

float Camera::GetFarPlane() const
{
    return m_FarPlane;
}

void Camera::SetFarPlane(float const Value)
{
    if (m_FarPlane != Value)
    {
        m_FarPlane      = Value;
        m_IsRenderDirty = true;
    }
}

float Camera::GetDrawDistance() const
{
    return m_DrawDistance;
}

void Camera::SetDrawDistance(float const Value)
{
    if (m_DrawDistance != Value)
    {
        m_DrawDistance  = Value;
        m_IsRenderDirty = true;
    }
}

glm::vec3 Camera::GetFront() const
{
    float const Yaw   = glm::radians(m_Rotation.x);
    float const Pitch = glm::radians(m_Rotation.y);
    return glm::vec3 { cos(Yaw) * cos(Pitch), sin(Pitch), sin(Yaw) * cos(Pitch) };
}

glm::vec3 Camera::GetRight() const
{
    float const Yaw = glm::radians(m_Rotation.x);
    return glm::vec3 { cos(Yaw - glm::radians(90.0f)), 0.0f, sin(Yaw - glm::radians(90.0f)) };
}

glm::vec3 Camera::GetUp() const
{
    return cross(GetFront(), GetRight());
}

glm::mat4 Camera::GetViewMatrix() const
{
    return lookAt(m_Position, m_Position + GetFront(), GetUp());
}

glm::mat4 Camera::GetProjectionMatrix() const
{
    VkExtent2D const &SwapChainExtent = GetSwapChainExtent();
    glm::mat4         Projection      = glm::perspective(glm::radians(m_FieldOfView),
                                                         static_cast<float>(SwapChainExtent.width) / static_cast<float>(SwapChainExtent.height),
                                                         m_NearPlane,
                                                         m_FarPlane);
    Projection[1][1] *= -1;

    return Projection;
}

CameraMovementStateFlags Camera::GetCameraMovementStateFlags() const
{
    return m_MovementStateFlags;
}

void Camera::SetCameraMovementStateFlags(CameraMovementStateFlags const Value)
{
    m_MovementStateFlags = Value;
}

void Camera::UpdateCameraMovement(float const DeltaTime)
{
    float const     CameraSpeed = GetSpeed();
    glm::vec3 const CameraFront = GetFront();
    glm::vec3 const CameraUp    = GetUp();
    glm::vec3 const CameraRight = GetRight();
    glm::vec3       NewPosition = GetPosition();

    if (HasFlag(m_MovementStateFlags, CameraMovementStateFlags::FORWARD))
    {
        NewPosition += CameraSpeed * CameraFront * DeltaTime;
    }

    if (HasFlag(m_MovementStateFlags, CameraMovementStateFlags::BACKWARD))
    {
        NewPosition -= CameraSpeed * CameraFront * DeltaTime;
    }

    if (HasFlag(m_MovementStateFlags, CameraMovementStateFlags::LEFT))
    {
        NewPosition += CameraSpeed * CameraRight * DeltaTime;
    }

    if (HasFlag(m_MovementStateFlags, CameraMovementStateFlags::RIGHT))
    {
        NewPosition -= CameraSpeed * CameraRight * DeltaTime;
    }

    if (HasFlag(m_MovementStateFlags, CameraMovementStateFlags::UP))
    {
        NewPosition += CameraSpeed * CameraUp * DeltaTime;
    }

    if (HasFlag(m_MovementStateFlags, CameraMovementStateFlags::DOWN))
    {
        NewPosition -= CameraSpeed * CameraUp * DeltaTime;
    }

    SetPosition(NewPosition);
}

bool Camera::IsInsideCameraFrustum(glm::vec3 const &TestLocation) const
{
    glm::mat4 const ViewProjectionMatrix = GetProjectionMatrix() * GetViewMatrix();

    auto const      HomogeneousTestLocation = glm::vec4(TestLocation, 1.F);
    glm::vec4 const ClipSpaceTestLocation   = ViewProjectionMatrix * HomogeneousTestLocation;

    return std::abs(ClipSpaceTestLocation.x) <= std::abs(ClipSpaceTestLocation.w) && std::abs(ClipSpaceTestLocation.y) <=
           std::abs(ClipSpaceTestLocation.w) && std::abs(ClipSpaceTestLocation.z) <= std::abs(ClipSpaceTestLocation.w);
}

bool Camera::IsInAllowedDistance(glm::vec3 const &TestLocation) const
{
    glm::vec3 const CameraToTestLocation   = TestLocation - GetPosition();
    float const     DistanceToTestLocation = length(CameraToTestLocation);

    return DistanceToTestLocation <= GetDrawDistance();
}

bool Camera::CanDrawObject(std::shared_ptr<Object> const &Object) const
{
    if (Object->IsPendingDestroy())
    {
        return false;
    }

    if constexpr (g_EnableExperimentalFrustumCulling)
    {
        return IsInsideCameraFrustum(Object->GetPosition()) && IsInAllowedDistance(Object->GetPosition());
    }

    return true;
}

bool Camera::IsRenderDirty() const
{
    return m_IsRenderDirty;
}

void Camera::SetRenderDirty(bool const Value) const
{
    m_IsRenderDirty = Value;
}
