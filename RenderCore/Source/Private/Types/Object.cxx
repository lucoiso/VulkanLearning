// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <algorithm>
#include <Volk/volk.h>
#include <glm/ext.hpp>
#include <string>
#include <vector>

module RenderCore.Types.Object;

import RenderCore.Renderer;
import RenderCore.Runtime.Memory;
import RenderCore.Runtime.Pipeline;
import RenderCore.Runtime.Device;
import RenderCore.Runtime.Scene;
import RenderCore.Utils.Constants;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Types.Material;

using namespace RenderCore;

Object::Object(std::uint32_t const ID, std::string_view const Path)
    : Resource(ID, Path)
{
}

Object::Object(std::uint32_t const ID, std::string_view const Path, std::string_view const Name)
    : Resource(ID, Path, Name)
{
}

Transform const &Object::GetTransform() const
{
    return m_Transform;
}

void Object::SetTransform(Transform const &Value)
{
    if (m_Transform != Value)
    {
        m_Transform     = Value;
        m_IsRenderDirty = true;
    }
}

glm::vec3 Object::GetPosition() const
{
    return m_Transform.GetPosition();
}

void Object::SetPosition(glm::vec3 const &Value)
{
    if (m_Transform.GetPosition() != Value)
    {
        m_Transform.SetPosition(Value);
        m_IsRenderDirty = true;
    }
}

glm::vec3 Object::GetRotation() const
{
    return m_Transform.GetRotation();
}

void Object::SetRotation(glm::vec3 const &Value)
{
    if (m_Transform.GetRotation() != Value)
    {
        m_Transform.SetRotation(Value);
        m_IsRenderDirty = true;
    }
}

glm::vec3 Object::GetScale() const
{
    return m_Transform.GetScale();
}

void Object::SetScale(glm::vec3 const &Value)
{
    if (m_Transform.GetScale() != Value)
    {
        m_Transform.SetScale(Value);
        m_IsRenderDirty = true;
    }
}

glm::mat4 Object::GetMatrix() const
{
    return m_Transform.GetMatrix();
}

void Object::SetMatrix(glm::mat4 const &Value)
{
    m_Transform.SetMatrix(Value);
    m_IsRenderDirty = true;
}

void Object::Destroy()
{
    Resource::Destroy();
}

std::uint32_t Object::GetUniformOffset() const
{
    return m_UniformOffset;
}

void Object::SetUniformOffset(std::uint32_t const &Offset)
{
    m_UniformOffset = Offset;
}

void Object::SetupUniformDescriptor()
{
    m_UniformBufferInfo = GetAllocationBufferDescriptor(GetBufferIndex(), m_UniformOffset, sizeof(ModelUniformData));
}

void Object::UpdateUniformBuffers() const
{
    void *AllocationData = GetAllocationMappedData(GetBufferIndex());

    if (!AllocationData)
    {
        return;
    }

    if (m_IsRenderDirty)
    {
        constexpr auto ModelUBOSize = sizeof(ModelUniformData);

        ModelUniformData const UpdatedModelUBO {
                .Model = m_Transform.GetMatrix() * m_Mesh->GetTransform().GetMatrix(),
                .BaseColorFactor = m_Mesh->GetMaterialData().BaseColorFactor,
                .EmissiveFactor = m_Mesh->GetMaterialData().EmissiveFactor,
                .MetallicFactor = static_cast<double>(m_Mesh->GetMaterialData().MetallicFactor),
                .RoughnessFactor = static_cast<double>(m_Mesh->GetMaterialData().RoughnessFactor),
                .AlphaCutoff = static_cast<double>(m_Mesh->GetMaterialData().AlphaCutoff),
                .NormalScale = static_cast<double>(m_Mesh->GetMaterialData().NormalScale),
                .OcclusionStrength = static_cast<double>(m_Mesh->GetMaterialData().OcclusionStrength),
                .AlphaMode = static_cast<int>(m_Mesh->GetMaterialData().AlphaMode),
                .DoubleSided = static_cast<int>(m_Mesh->GetMaterialData().DoubleSided)
        };

        std::memcpy(static_cast<char *>(AllocationData) + GetUniformOffset(), &UpdatedModelUBO, ModelUBOSize);
        m_IsRenderDirty = false;
    }
}

void Object::DrawObject(VkCommandBuffer const &CommandBuffer) const
{
    if (!m_Mesh)
    {
        return;
    }

    std::vector WriteDescriptors {
            VkWriteDescriptorSet {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = VK_NULL_HANDLE,
                    .dstBinding = 0U,
                    .dstArrayElement = 0U,
                    .descriptorCount = 1U,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &GetSceneUniformDescriptor(),
            },
            VkWriteDescriptorSet {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = VK_NULL_HANDLE,
                    .dstBinding = 1U,
                    .dstArrayElement = 0U,
                    .descriptorCount = 1U,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &m_UniformBufferInfo,
            }
    };

    WriteDescriptors.append_range(m_Mesh->GetWriteDescriptorSet());

    vkCmdPushDescriptorSetKHR(CommandBuffer,
                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                              GetPipelineLayout(),
                              0U,
                              static_cast<std::uint32_t>(std::size(WriteDescriptors)),
                              std::data(WriteDescriptors));

    m_Mesh->BindBuffers(CommandBuffer);
}

std::shared_ptr<Mesh> Object::GetMesh() const
{
    return m_Mesh;
}

void Object::SetMesh(std::shared_ptr<Mesh> const &Value)
{
    m_Mesh = Value;
}

bool Object::IsRenderDirty() const
{
    return m_IsRenderDirty;
}

std::vector<VkWriteDescriptorSet> Object::GetWriteDescriptorSet() const
{
    return Resource::GetWriteDescriptorSet();
}
