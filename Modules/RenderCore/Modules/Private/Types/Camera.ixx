// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <glm/ext.hpp>

export module RenderCore.Types.Camera;

import RenderCore.Types.Transform;

namespace RenderCore
{
    export enum class CameraMovementStateFlags : std::uint8_t {
        NONE     = 0,
        FORWARD  = 1 << 1,
        BACKWARD = 1 << 2,
        LEFT     = 1 << 3,
        RIGHT    = 1 << 4,
        UP       = 1 << 5,
        DOWN     = 1 << 6
    };

    export class Camera
    {
        Vector m_CameraPosition {0.F, 0.F, 3.F};
        Rotator m_CameraRotation {0.F, -90.F, 0.F};

        float m_CameraSpeed {0.005F};
        float m_CameraSensitivity {0.1F};

        CameraMovementStateFlags m_CameraMovementStateFlags {CameraMovementStateFlags::NONE};

    public:
        [[nodiscard]] Vector GetPosition() const;
        void SetPosition(Vector const&);

        [[nodiscard]] Rotator GetRotation() const;
        void SetRotation(Rotator const&);

        [[nodiscard]] float GetSpeed() const;
        void SetSpeed(float);

        [[nodiscard]] float GetSensitivity() const;
        void SetSensitivity(float);

        [[nodiscard]] glm::mat4 GetMatrix() const;

        [[nodiscard]] CameraMovementStateFlags GetCameraMovementStateFlags();
        void SetCameraMovementStateFlags(CameraMovementStateFlags);
        void UpdateCameraMovement(GLFWwindow*, float);
    };

    export [[nodiscard]] Camera& GetViewportCamera();
}// namespace RenderCore