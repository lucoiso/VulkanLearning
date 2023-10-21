// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include "RenderCoreModule.h"

#include <glm/ext.hpp>
#include <utility>

export module RenderCore.Types.Object;

import <cstdint>;
import <string>;

import RenderCore.Types.Transform;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Object
    {
        std::uint32_t m_ID {};
        std::string m_Name {};
        std::string m_Path {};
        Transform m_Transform {};

    public:
        Object() = delete;

        Object(Object const&)            = default;
        Object& operator=(Object const&) = default;

        Object(std::uint32_t const ID, std::string_view const Path)
            : m_ID(ID), m_Path(Path)
        {
            m_Name = m_Path.substr(m_Path.find_last_of('/') + 1, m_Path.find_last_of('.') - m_Path.find_last_of('/') - 1);
        }

        [[nodiscard]] std::uint32_t GetID() const
        {
            return m_ID;
        }

        [[nodiscard]] std::string_view GetPath() const
        {
            return m_Path;
        }

        [[nodiscard]] std::string_view GetName() const
        {
            return m_Name;
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