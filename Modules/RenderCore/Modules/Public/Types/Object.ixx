// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include "RenderCoreModule.h"

#include <glm/ext.hpp>

export module RenderCore.Types.Object;
export import RenderCore.Types.Transform;

import <cstdint>;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Object
    {
        std::uint32_t m_ID {};
        Transform m_Transform {};

    public:
        Object()                         = delete;
        Object(Object const&)            = delete;
        Object& operator=(Object const&) = delete;

        explicit Object(std::uint32_t const ID)
            : m_ID(ID)
        {
        }

        [[nodiscard]] std::uint32_t GetID() const
        {
            return m_ID;
        }

        virtual ~Object() = default;

        [[nodiscard]] Transform& GetTransform()
        {
            return m_Transform;
        }

        [[nodiscard]] Transform const& GetTransform() const
        {
            return m_Transform;
        }

        void SetTransform(Transform const& Value)
        {
            m_Transform = Value;
        }

        [[nodiscard]] Vector GetPosition() const
        {
            return m_Transform.Position;
        }

        void SetPosition(Vector const& Position)
        {
            m_Transform.Position = Position;
        }

        [[nodiscard]] Rotator GetRotation() const
        {
            return m_Transform.Rotation;
        }

        void SetRotation(Rotator const& Rotation)
        {
            m_Transform.Rotation = Rotation;
        }

        [[nodiscard]] Vector GetScale() const
        {
            return m_Transform.Scale;
        }

        void SetScale(Vector const& Scale)
        {
            m_Transform.Scale = Scale;
        }

        [[nodiscard]] glm::mat4 GetMatrix() const
        {
            return m_Transform.ToGlmMat4();
        }
    };
}// namespace RenderCore