// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <glm/ext.hpp>

module RenderCore.Types.Camera;

import RenderCore.Runtime.Buffer;
import RenderCore.Utils.EnumHelpers;
import RenderCore.Utils.Constants;

using namespace RenderCore;

Vector Camera::GetPosition() const
{
    return m_CameraPosition;
}

void Camera::SetPosition(Vector const& Position)
{
    m_CameraPosition = Position;
}

Rotator Camera::GetRotation() const
{
    return m_CameraRotation;
}

void Camera::SetRotation(Rotator const& Rotation)
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

glm::mat4 Camera::GetViewMatrix() const
{
    return lookAt(m_CameraPosition.ToGlmVec3(), (m_CameraPosition + m_CameraRotation.GetFront()).ToGlmVec3(), m_CameraRotation.GetUp().ToGlmVec3());
}

glm::mat4 Camera::GetProjectionMatrix(VkExtent2D const& Extent) const
{
    glm::mat4 Projection = glm::perspective(glm::radians(m_FieldOfView),
                                            static_cast<float>(Extent.width) / static_cast<float>(Extent.height),
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
    float const CameraSpeed{
        GetSpeed()
    };
    Vector const CameraFront{
        m_CameraRotation.GetFront()
    };
    Vector const CameraRight{
        m_CameraRotation.GetRight()
    };

    Vector NewPosition{
        GetPosition()
    };

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

bool Camera::IsInsideCameraFrustum(Vector const& TestLocation,
                                   VkExtent2D const& Extent) const
{
    glm::mat4 const ViewProjectionMatrix = GetProjectionMatrix(Extent) * GetViewMatrix();

    auto const HomogeneousTestLocation = glm::vec4(TestLocation.ToGlmVec3(), 1.F);
    glm::vec4 const ClipSpaceTestLocation = ViewProjectionMatrix * HomogeneousTestLocation;

    return std::abs(ClipSpaceTestLocation.x) <= std::abs(ClipSpaceTestLocation.w) && std::abs(ClipSpaceTestLocation.y) <= std::abs(ClipSpaceTestLocation.w) &&
        std::abs(ClipSpaceTestLocation.z) <= std::abs(ClipSpaceTestLocation.w);
}

bool Camera::IsInAllowedDistance(Vector const& TestLocation) const
{
    Vector const CameraToTestLocation = TestLocation - GetPosition();
    float const DistanceToTestLocation = CameraToTestLocation.Length();

    return DistanceToTestLocation <= GetDrawDistance();
}

bool Camera::CanDrawObject(std::shared_ptr<Object> const& Object,
                           VkExtent2D const& Extent) const
{
    if (!Object)
    {
        return false;
    }

    if constexpr (g_EnableExperimentalFrustumCulling)
    {
        return IsInsideCameraFrustum(Object->GetPosition(), Extent) && IsInAllowedDistance(Object->GetPosition());
    }

    return true;
}
