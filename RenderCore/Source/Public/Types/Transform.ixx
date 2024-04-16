// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <glm/ext.hpp>
#include "RenderCoreModule.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

export module RenderCore.Types.Transform;

namespace RenderCore
{
    export struct RENDERCOREMODULE_API Bounds
    {
        glm::vec3 Min { FLT_MAX };
        glm::vec3 Max { FLT_MIN };
    };

    export class RENDERCOREMODULE_API Transform
    {
        glm::vec3 m_Position { 0.F };
        glm::vec3 m_Scale { 1.F };
        glm::vec3 m_Rotation { 0.F };

    public:
        Transform() = default;

        [[nodiscard]] inline glm::vec3 GetPosition() const
        {
            return m_Position;
        }

        inline void SetPosition(glm::vec3 const &Value)
        {
            m_Position = Value;
        }

        [[nodiscard]] inline glm::vec3 GetRotation() const
        {
            return m_Rotation;
        }

        inline void SetRotation(glm::vec3 const &Value)
        {
            m_Rotation = Value;
        }

        [[nodiscard]] inline glm::vec3 GetScale() const
        {
            return m_Scale;
        }

        inline void SetScale(glm::vec3 const &Value)
        {
            m_Scale = Value;
        }

        [[nodiscard]] inline glm::mat4 GetMatrix() const
        {
            glm::mat4 Matrix { 1.F };
            Matrix = translate(Matrix, m_Position);
            Matrix = scale(Matrix, m_Scale);
            Matrix = rotate(Matrix, glm::radians(m_Rotation.x), glm::vec3(1.F, 0.F, 0.F));
            Matrix = rotate(Matrix, glm::radians(m_Rotation.y), glm::vec3(0.F, 1.F, 0.F));
            Matrix = rotate(Matrix, glm::radians(m_Rotation.z), glm::vec3(0.F, 0.F, 1.F));

            return Matrix;
        }

        void SetMatrix(glm::mat4 const &Matrix)
        {
            glm::quat Quaternion;
            glm::vec3 _1;
            glm::vec4 _2;
            decompose(Matrix, m_Scale, Quaternion, m_Position, _1, _2);

            m_Rotation = eulerAngles(Quaternion);
        }

        bool operator==(Transform const &Rhs) const
        {
            return m_Position == Rhs.m_Position && m_Scale == Rhs.m_Scale && m_Rotation == Rhs.m_Rotation;
        }

        bool operator!=(Transform const &Rhs) const
        {
            return !(*this == Rhs);
        }
    };
} // namespace RenderCore
