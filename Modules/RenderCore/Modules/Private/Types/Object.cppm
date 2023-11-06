// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <glm/ext.hpp>
#include <utility>

module RenderCore.Types.Object;

import RenderCore.EngineCore;

using namespace RenderCore;

RenderCore::Object::Object(std::uint32_t const ID, std::string_view const Path)
    : m_ID(ID),
      m_Path(Path),
      m_Name(m_Path.substr(m_Path.find_last_of('/') + 1, m_Path.find_last_of('.') - m_Path.find_last_of('/') - 1))
{
}

RenderCore::Object::~Object()
{
    EngineCore::Get().UnloadScene({GetID()});
}

[[nodiscard]] std::uint32_t RenderCore::Object::GetID() const
{
    return m_ID;
}

[[nodiscard]] std::string_view RenderCore::Object::GetPath() const
{
    return m_Path;
}

[[nodiscard]] std::string_view RenderCore::Object::GetName() const
{
    return m_Name;
}

[[nodiscard]] Transform& RenderCore::Object::GetMutableTransform()
{
    return m_Transform;
}

[[nodiscard]] Transform const& RenderCore::Object::GetTransform() const
{
    return m_Transform;
}

void RenderCore::Object::SetTransform(Transform const& Value)
{
    m_Transform = Value;
}

[[nodiscard]] Vector RenderCore::Object::GetPosition() const
{
    return m_Transform.Position;
}

void RenderCore::Object::SetPosition(Vector const& Position)
{
    m_Transform.Position = Position;
}

[[nodiscard]] Rotator RenderCore::Object::GetRotation() const
{
    return m_Transform.Rotation;
}

void RenderCore::Object::SetRotation(Rotator const& Rotation)
{
    m_Transform.Rotation = Rotation;
}

[[nodiscard]] Vector RenderCore::Object::GetScale() const
{
    return m_Transform.Scale;
}

void RenderCore::Object::SetScale(Vector const& Scale)
{
    m_Transform.Scale = Scale;
}

[[nodiscard]] glm::mat4 RenderCore::Object::GetMatrix() const
{
    return m_Transform.ToGlmMat4();
}

void RenderCore::Object::Destroy()
{
    EngineCore::Get().UnloadScene({GetID()});
}