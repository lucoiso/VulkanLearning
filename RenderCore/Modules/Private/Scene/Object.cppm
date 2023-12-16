// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <glm/ext.hpp>
#include <utility>
#include <string>

module RenderCore.Types.Object;

import RenderCore.Renderer;

using namespace RenderCore;

void Object::SetTrianglesCount(std::uint32_t const TrianglesCount)
{
    m_TrianglesCount = TrianglesCount;
}

Object::Object(std::uint32_t const ID, std::string const& Path)
    : m_ID(ID),
      m_Path(Path),
      m_Name(m_Path.substr(m_Path.find_last_of('/') + 1, m_Path.find_last_of('.') - m_Path.find_last_of('/') - 1))
{
}

Object::Object(std::uint32_t const ID, std::string const& Path, std::string const& Name)
    : m_ID(ID),
      m_Path(Path),
      m_Name(Name)
{
}
std::uint32_t Object::GetID() const
{
    return m_ID;
}

std::string const& Object::GetPath() const
{
    return m_Path;
}

std::string const& Object::GetName() const
{
    return m_Name;
}

std::uint32_t Object::GetTrianglesCount() const
{
    return m_TrianglesCount;
}

Transform& Object::GetMutableTransform()
{
    return m_Transform;
}

Transform const& Object::GetTransform() const
{
    return m_Transform;
}

void Object::SetTransform(Transform const& Value)
{
    m_Transform = Value;
}

Vector Object::GetPosition() const
{
    return m_Transform.Position;
}

void Object::SetPosition(Vector const& Position)
{
    m_Transform.Position = Position;
}

Rotator Object::GetRotation() const
{
    return m_Transform.Rotation;
}

void Object::SetRotation(Rotator const& Rotation)
{
    m_Transform.Rotation = Rotation;
}

Vector Object::GetScale() const
{
    return m_Transform.Scale;
}

void Object::SetScale(Vector const& Scale)
{
    m_Transform.Scale = Scale;
}

glm::mat4 Object::GetMatrix() const
{
    return m_Transform.ToGlmMat4();
}

bool Object::IsPendingDestroy() const
{
    return m_IsPendingDestroy;
}

void Object::Destroy()
{
    m_IsPendingDestroy = true;
}