// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <glm/ext.hpp>

module RenderCore.Types.Illumination;

using namespace RenderCore;

void Illumination::SetPosition(glm::vec3 const &Value)
{
    m_LightPosition = Value;
}

glm::vec3 const &Illumination::GetPosition() const
{
    return m_LightPosition;
}

void Illumination::SetColor(glm::vec3 const &Value)
{
    m_LightColor = Value;
}

glm::vec3 const &Illumination::GetColor() const
{
    return m_LightColor;
}

void Illumination::SetIntensity(const float Value)
{
    m_LightIntensity = Value;
}

float Illumination::GetIntensity() const
{
    return m_LightIntensity;
}
