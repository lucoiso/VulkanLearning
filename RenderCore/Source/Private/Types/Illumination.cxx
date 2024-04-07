// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <glm/ext.hpp>

module RenderCore.Types.Illumination;

import RenderCore.Runtime.Memory;
import RenderCore.Types.UniformBufferObject;

using namespace RenderCore;

void Illumination::Destroy()
{
    m_UniformBufferAllocation.first.DestroyResources(GetAllocator());
}

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

void Illumination::SetIntensity(const float Value)
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

void *Illumination::GetUniformData() const
{
    return m_UniformBufferAllocation.first.MappedData;
}

VkDescriptorBufferInfo const &Illumination::GetUniformDescriptor() const
{
    return m_UniformBufferAllocation.second;
}

void Illumination::UpdateUniformBuffers()
{
    if (!m_IsRenderDirty)
    {
        return;
    }

    if (m_UniformBufferAllocation.first.MappedData)
    {
        LightUniformData const UpdatedUBO { .LightPosition = GetPosition(), .LightColor = GetColor() * GetIntensity() };
        std::memcpy(m_UniformBufferAllocation.first.MappedData, &UpdatedUBO, sizeof(LightUniformData));
    }

    m_IsRenderDirty = false;
}

void Illumination::AllocateUniformBuffer()
{
    constexpr VkDeviceSize BufferSize = sizeof(LightUniformData);
    CreateUniformBuffers(m_UniformBufferAllocation.first, BufferSize, "LIGHT_UNIFORM");
    m_UniformBufferAllocation.second = { .buffer = m_UniformBufferAllocation.first.Buffer, .offset = 0U, .range = BufferSize };
}
