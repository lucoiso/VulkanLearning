// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <glm/ext.hpp>

module RenderCore.Types.Illumination;

using namespace RenderCore;

void Illumination::SetPosition(Vector const& Value)
{
    m_LightPosition = Value;
}

Vector const& Illumination::GetPosition() const
{
    return m_LightPosition;
}

void Illumination::SetColor(Vector const& Value)
{
    m_LightColor = Value;
}

Vector const& Illumination::GetColor() const
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