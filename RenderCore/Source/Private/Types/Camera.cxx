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

using namespace RenderCore;

glm::vec3 Camera::GetPosition() const
{
    return m_CameraPosition;
}

void Camera::SetPosition(glm::vec3 const &Position)
{
    m_CameraPosition = Position;
}

glm::vec3 Camera::GetRotation() const
{
    return m_CameraRotation;
}

void Camera::SetRotation(glm::vec3 const &Rotation)
{
    m_CameraRotation = Rotation;
}

float Camera::GetSpeed() const
{
    return m_CameraSpeed;
}

void Camera::SetSpeed(float const Speed)
{
    m_CameraSpeed = Speed;
}

float Camera::GetSensitivity() const
{
    return m_CameraSensitivity;
}

void Camera::SetSensitivity(float const Sensitivity)
{
    m_CameraSensitivity = Sensitivity;
}

float Camera::GetFieldOfView() const
{
    return m_FieldOfView;
}

void Camera::SetFieldOfView(float const FieldOfView)
{
    m_FieldOfView = FieldOfView;
}

float Camera::GetNearPlane() const
{
    return m_NearPlane;
}

void Camera::SetNearPlane(float const NearPlane)
{
    m_NearPlane = NearPlane;
}

float Camera::GetFarPlane() const
{
    return m_FarPlane;
}

void Camera::SetFarPlane(float const FarPlane)
{
    m_FarPlane = FarPlane;
}

float Camera::GetDrawDistance() const
{
    return m_DrawDistance;
}

void Camera::SetDrawDistance(float const DrawDistance)
{
    m_DrawDistance = DrawDistance;
}

glm::vec3 Camera::GetFront() const
{
    float const Yaw   = glm::radians(m_CameraRotation.y);
    float const Pitch = glm::radians(m_CameraRotation.x);

    glm::vec3 Front;
    Front.x = cos(Yaw) * cos(Pitch);
    Front.y = sin(Pitch);
    Front.z = sin(Yaw) * cos(Pitch);

    return normalize(Front);
}

glm::vec3 Camera::GetUp() const
{
    return rotate(glm::mat4(1.0f), glm::radians(m_CameraRotation.z), GetRight()) * glm::vec4 { 0.F, 1.F, 0.F, 1.F };
}

glm::vec3 Camera::GetRight() const
{
    return cross(GetFront(), glm::vec3 { 0.F, 1.F, 0.F });
}

glm::mat4 Camera::GetViewMatrix() const
{
    return lookAt(m_CameraPosition, m_CameraPosition + GetFront(), GetUp());
}

glm::mat4 Camera::GetProjectionMatrix() const
{
    VkExtent2D const &SwapChainExtent = GetSwapChainExtent();

    glm::mat4 Projection = glm::perspective(glm::radians(m_FieldOfView),
                                            static_cast<float>(SwapChainExtent.width) / static_cast<float>(SwapChainExtent.height),
                                            m_NearPlane,
                                            m_FarPlane);
    Projection[1][1] *= -1;

    return Projection;
}

CameraMovementStateFlags Camera::GetCameraMovementStateFlags() const
{
    return m_CameraMovementStateFlags;
}

void Camera::SetCameraMovementStateFlags(CameraMovementStateFlags const State)
{
    m_CameraMovementStateFlags = State;
}

void Camera::UpdateCameraMovement(float const DeltaTime)
{
    float const     CameraSpeed { GetSpeed() };
    glm::vec3 const CameraFront { GetFront() };
    glm::vec3 const CameraUp { GetUp() };
    glm::vec3 const CameraRight { GetRight() };
    glm::vec3       NewPosition { GetPosition() };

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
        NewPosition -= CameraSpeed * CameraRight * DeltaTime;
    }

    if (HasFlag(m_CameraMovementStateFlags, CameraMovementStateFlags::RIGHT))
    {
        NewPosition += CameraSpeed * CameraRight * DeltaTime;
    }

    if (HasFlag(m_CameraMovementStateFlags, CameraMovementStateFlags::UP))
    {
        NewPosition += CameraSpeed * CameraUp * DeltaTime;
    }

    if (HasFlag(m_CameraMovementStateFlags, CameraMovementStateFlags::DOWN))
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

bool Camera::CanDrawObject(Object const &Object) const
{
    if (Object.IsPendingDestroy())
    {
        return false;
    }

    if constexpr (g_EnableExperimentalFrustumCulling)
    {
        return IsInsideCameraFrustum(Object.GetPosition()) && IsInAllowedDistance(Object.GetPosition());
    }

    return true;
}
