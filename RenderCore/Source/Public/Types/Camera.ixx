// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <memory>
#include <Volk/volk.h>
#include <glm/ext.hpp>
#include "RenderCoreModule.hpp"

export module RenderCore.Types.Camera;

import RenderCore.Types.Transform;
import RenderCore.Types.Object;
import RenderCore.Types.Allocation;

namespace RenderCore
{
    export enum class CameraMovementStateFlags : std::uint8_t
    {
        NONE     = 0,
        FORWARD  = 1 << 1,
        BACKWARD = 1 << 2,
        LEFT     = 1 << 3,
        RIGHT    = 1 << 4,
        UP       = 1 << 5,
        DOWN     = 1 << 6
    };

    export class RENDERCOREMODULE_API Camera
    {
        mutable bool             m_IsRenderDirty { true };
        CameraMovementStateFlags m_MovementStateFlags { CameraMovementStateFlags::NONE };
        float                    m_Speed { 1.F };
        float                    m_Sensitivity { 1.F };
        float                    m_FieldOfView { 45.F };
        float                    m_NearPlane { 0.001F };
        float                    m_FarPlane { 1000.F };
        float                    m_CurrentAspectRatio { 1.F };
        float                    m_DrawDistance { 500.F };
        glm::vec3                m_Position { 0.F, 0.F, 1.F };
        glm::vec3                m_Rotation { -90.F, 0.F, 0.F };

    public:
        Camera() = default;

        [[nodiscard]] glm::vec3 GetPosition() const;
        void                    SetPosition(glm::vec3 const &);

        [[nodiscard]] glm::vec3 GetRotation() const;
        void                    SetRotation(glm::vec3 const &);

        [[nodiscard]] float GetSpeed() const;
        void                SetSpeed(float);

        [[nodiscard]] float GetSensitivity() const;
        void                SetSensitivity(float);

        [[nodiscard]] float GetFieldOfView() const;
        void                SetFieldOfView(float);

        [[nodiscard]] float GetNearPlane() const;
        void                SetNearPlane(float);

        [[nodiscard]] float GetFarPlane() const;
        void                SetFarPlane(float);

        [[nodiscard]] float GetDrawDistance() const;
        void                SetDrawDistance(float);

        [[nodiscard]] glm::vec3 GetFront() const;
        [[nodiscard]] glm::vec3 GetUp() const;
        [[nodiscard]] glm::vec3 GetRight() const;

        [[nodiscard]] glm::mat4 GetViewMatrix() const;
        [[nodiscard]] glm::mat4 GetProjectionMatrix() const;

        [[nodiscard]] CameraMovementStateFlags GetCameraMovementStateFlags() const;
        void                                   SetCameraMovementStateFlags(CameraMovementStateFlags);
        void                                   UpdateCameraMovement(float);

        [[nodiscard]] bool IsInsideCameraFrustum(glm::vec3 const &) const;
        [[nodiscard]] bool IsInAllowedDistance(glm::vec3 const &) const;
        [[nodiscard]] bool CanDrawObject(std::shared_ptr<Object> const &) const;

        [[nodiscard]] bool IsRenderDirty() const;
        void               SetRenderDirty(bool) const;
    };
} // namespace RenderCore
