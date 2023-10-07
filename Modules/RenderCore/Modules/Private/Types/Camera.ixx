// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#pragma once

#include <GLFW/glfw3.h>
#include <glm/ext.hpp>

export module RenderCore.Types.Camera;

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
        glm::vec3 m_CameraPosition {0.F, 0.F, 3.F};
        glm::vec3 m_CameraFront {0.F, 0.F, -1.F};
        glm::vec3 m_CameraUp {0.F, 1.F, 0.F};
        float m_CameraSpeed {0.01F};
        float m_CameraYaw {-90.F};
        float m_CameraPitch {0.F};
        float m_CameraSensitivity {0.1F};
        CameraMovementStateFlags m_CameraMovementStateFlags {CameraMovementStateFlags::NONE};

    public:
        [[nodiscard]] glm::vec3 GetPosition() const;
        void SetPosition(glm::vec3 const&);

        [[nodiscard]] glm::vec3 GetFront() const;
        void SetFront(glm::vec3 const&);

        [[nodiscard]] glm::vec3 GetUp() const;
        void SetUp(glm::vec3 const&);

        [[nodiscard]] float GetSpeed() const;
        void SetSpeed(float);

        [[nodiscard]] float GetYaw() const;
        void SetYaw(float);

        [[nodiscard]] float GetPitch() const;
        void SetPitch(float);

        [[nodiscard]] float GetSensitivity() const;
        void SetSensitivity(float);

        [[nodiscard]] glm::mat4 GetMatrix() const;

        [[nodiscard]] CameraMovementStateFlags GetCameraMovementStateFlags();
        void SetCameraMovementStateFlags(CameraMovementStateFlags);
        void UpdateCameraMovement(GLFWwindow*, float);
    };

    export [[nodiscard]] Camera& GetViewportCamera();
}// namespace RenderCore