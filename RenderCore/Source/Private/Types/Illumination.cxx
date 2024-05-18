// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <glm/ext.hpp>

module RenderCore.Types.Illumination;

import RenderCore.Runtime.Memory;
import RenderCore.Types.UniformBufferObject;

using namespace RenderCore;

void Illumination::SetPosition(glm::vec3 const &Value)
{
    if (m_Position != Value)
    {
        m_Position      = Value;
        m_IsRenderDirty = true;
    }
}

glm::vec3 const &Illumination::GetPosition() const
{
    return m_Position;
}

void Illumination::SetColor(glm::vec3 const &Value)
{
    if (m_Color != Value)
    {
        m_Color         = Value;
        m_IsRenderDirty = true;
    }
}

glm::vec3 const &Illumination::GetColor() const
{
    return m_Color;
}

void Illumination::SetIntensity(float const Value)
{
    if (m_Intensity != Value)
    {
        m_Intensity     = Value;
        m_IsRenderDirty = true;
    }
}

float Illumination::GetIntensity() const
{
    return m_Intensity;
}

void Illumination::SetAmbient(float const Value)
{
    if (m_Ambient != Value)
    {
        m_Ambient       = Value;
        m_IsRenderDirty = true;
    }
}

float Illumination::GetAmbient() const
{
    return m_Ambient;
}

bool Illumination::IsRenderDirty() const
{
    return m_IsRenderDirty;
}

void Illumination::SetRenderDirty(bool const Value) const
{
    m_IsRenderDirty = Value;
}
