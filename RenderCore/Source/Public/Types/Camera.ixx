// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Types.Camera;

export import RenderCore.Types.Object;

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

        [[nodiscard]] inline glm::vec3 GetPosition() const
        {
            return m_Position;
        }

        inline void SetPosition(glm::vec3 const &Value)
        {
            if (m_Position != Value)
            {
                m_Position      = Value;
                m_IsRenderDirty = true;
            }
        }

        [[nodiscard]] inline glm::vec3 GetRotation() const
        {
            return m_Rotation;
        }

        inline void SetRotation(glm::vec3 const &Value)
        {
            if (m_Rotation != Value)
            {
                m_Rotation      = Value;
                m_IsRenderDirty = true;
            }
        }

        [[nodiscard]] inline float GetSpeed() const
        {
            return m_Speed;
        }

        inline void SetSpeed(float const Value)
        {
            m_Speed = Value;
        }

        [[nodiscard]] inline float GetSensitivity() const
        {
            return m_Sensitivity;
        }

        inline void SetSensitivity(float const Value)
        {
            m_Sensitivity = Value;
        }

        [[nodiscard]] inline float GetFieldOfView() const
        {
            return m_FieldOfView;
        }

        inline void SetFieldOfView(float const Value)
        {
            if (m_FieldOfView != Value)
            {
                m_FieldOfView   = Value;
                m_IsRenderDirty = true;
            }
        }

        [[nodiscard]] inline float GetNearPlane() const
        {
            return m_NearPlane;
        }

        inline void SetNearPlane(float const Value)
        {
            if (m_NearPlane != Value)
            {
                m_NearPlane     = Value;
                m_IsRenderDirty = true;
            }
        }

        [[nodiscard]] inline float GetFarPlane() const
        {
            return m_FarPlane;
        }

        inline void SetFarPlane(float const Value)
        {
            if (m_FarPlane != Value)
            {
                m_FarPlane      = Value;
                m_IsRenderDirty = true;
            }
        }

        [[nodiscard]] inline float GetDrawDistance() const
        {
            return m_DrawDistance;
        }

        inline void SetDrawDistance(float const Value)
        {
            if (m_DrawDistance != Value)
            {
                m_DrawDistance  = Value;
                m_IsRenderDirty = true;
            }
        }

        [[nodiscard]] glm::vec3 GetFront() const;
        [[nodiscard]] glm::vec3 GetUp() const;
        [[nodiscard]] glm::vec3 GetRight() const;

        [[nodiscard]] glm::mat4 GetViewMatrix() const;
        [[nodiscard]] glm::mat4 GetProjectionMatrix() const;


        [[nodiscard]] inline CameraMovementStateFlags GetCameraMovementStateFlags() const
        {
            return m_MovementStateFlags;
        }

        inline void SetCameraMovementStateFlags(CameraMovementStateFlags const Value)
        {
            m_MovementStateFlags = Value;
        }

        void UpdateCameraMovement(float);

        [[nodiscard]] bool        IsInsideCameraFrustum(std::shared_ptr<Object> const &) const;
        static void               CalculateFrustumPlanes(glm::mat4 const &, std::array<glm::vec4, 6U> &);
        [[nodiscard]] static bool BoxIntersectsPlane(Bounds const &, glm::vec4 const &);
        [[nodiscard]] bool        IsInAllowedDistance(std::shared_ptr<Object> const &) const;
        [[nodiscard]] bool        CanDrawObject(std::shared_ptr<Object> const &) const;

        [[nodiscard]] inline bool IsRenderDirty() const
        {
            return m_IsRenderDirty;
        }

        inline void SetRenderDirty(bool const Value) const
        {
            m_IsRenderDirty = Value;
        }
    };
} // namespace RenderCore
