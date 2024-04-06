// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <Volk/volk.h>
#include <glm/ext.hpp>
#include <string>

module RenderCore.Types.Object;

import RenderCore.Renderer;
import RenderCore.Runtime.Memory;
import RenderCore.Runtime.Pipeline;
import RenderCore.Runtime.Scene;
import RenderCore.Utils.Constants;
import RenderCore.Types.UniformBufferObject;

using namespace RenderCore;

Object::Object(std::uint32_t const ID, std::string_view const Path)
    : m_ID(ID)
  , m_Path(Path)
  , m_Name(m_Path.substr(m_Path.find_last_of('/') + 1, m_Path.find_last_of('.') - m_Path.find_last_of('/') - 1))
{
}

Object::Object(std::uint32_t const ID, std::string_view const Path, std::string_view const Name)
    : m_ID(ID)
  , m_Path(Path)
  , m_Name(Name)
{
}

std::uint32_t Object::GetID() const
{
    return m_ID;
}

std::string const &Object::GetPath() const
{
    return m_Path;
}

std::string const &Object::GetName() const
{
    return m_Name;
}

Transform &Object::GetMutableTransform()
{
    return m_Transform;
}

Transform const &Object::GetTransform() const
{
    return m_Transform;
}

void Object::SetTransform(Transform const &Value)
{
    m_Transform = Value;
}

glm::vec3 Object::GetPosition() const
{
    return m_Transform.GetPosition();
}

void Object::SetPosition(glm::vec3 const &Position)
{
    m_Transform.SetPosition(Position);
}

glm::vec3 Object::GetRotation() const
{
    return m_Transform.GetRotation();
}

void Object::SetRotation(glm::vec3 const &Rotation)
{
    m_Transform.SetRotation(Rotation);
}

glm::vec3 Object::GetScale() const
{
    return m_Transform.GetScale();
}

void Object::SetScale(glm::vec3 const &Scale)
{
    m_Transform.SetScale(Scale);
}

glm::mat4 Object::GetMatrix() const
{
    return m_Transform.GetMatrix();
}

void Object::SetMatrix(glm::mat4 const &Matrix)
{
    m_Transform.SetMatrix(Matrix);
}

MaterialData const &Object::GetMaterialData() const
{
    return m_MaterialData;
}

MaterialData &Object::GetMutableMaterialData()
{
    return m_MaterialData;
}

void Object::SetMaterialData(MaterialData const &Value)
{
    m_MaterialData = Value;
}

bool Object::IsPendingDestroy() const
{
    return m_IsPendingDestroy;
}

void Object::Destroy()
{
    m_IsPendingDestroy = true;
    m_Allocation.DestroyResources(GetAllocator());
}

ObjectAllocationData const &Object::GetAllocationData() const
{
    return m_Allocation;
}

ObjectAllocationData &Object::GetMutableAllocationData()
{
    return m_Allocation;
}

void Object::SetVertexBuffer(std::vector<Vertex> const &Value)
{
    m_Vertices = Value;
}

std::vector<Vertex> const &Object::GetVertices() const
{
    return m_Vertices;
}

std::vector<Vertex> &Object::GetMutableVertices()
{
    return m_Vertices;
}

void Object::SetIndexBuffer(std::vector<std::uint32_t> const &Value)
{
    m_Indices = Value;
}

std::vector<std::uint32_t> const &Object::GetIndices() const
{
    return m_Indices;
}

std::vector<std::uint32_t> &Object::GetMutableIndices()
{
    return m_Indices;
}

std::uint32_t Object::GetNumTriangles() const
{
    return static_cast<uint32_t>(std::size(m_Indices) / 3U);
}

void Object::UpdateUniformBuffers() const
{
    if (m_Allocation.ModelBufferAllocation.MappedData)
    {
        ModelUniformData const UpdatedModelUBO { .ModelMatrix = m_Transform.GetMatrix(), };

        std::memcpy(m_Allocation.ModelBufferAllocation.MappedData, &UpdatedModelUBO, sizeof(ModelUniformData));
    }

    if (m_Allocation.MaterialBufferAllocation.MappedData)
    {
        MaterialUniformData const UpdatedMaterialUBO {
                .BaseColorFactor = m_MaterialData.BaseColorFactor,
                .EmissiveFactor = m_MaterialData.EmissiveFactor,
                .MetallicFactor = static_cast<double>(m_MaterialData.MetallicFactor),
                .RoughnessFactor = static_cast<double>(m_MaterialData.RoughnessFactor),
                .AlphaCutoff = static_cast<double>(m_MaterialData.AlphaCutoff),
                .NormalScale = static_cast<double>(m_MaterialData.NormalScale),
                .OcclusionStrength = static_cast<double>(m_MaterialData.OcclusionStrength),
                .AlphaMode = static_cast<int>(m_MaterialData.AlphaMode),
                .DoubleSided = static_cast<int>(m_MaterialData.DoubleSided)
        };

        std::memcpy(m_Allocation.MaterialBufferAllocation.MappedData, &UpdatedMaterialUBO, sizeof(MaterialUniformData));
    }
}

void Object::DrawObject(VkCommandBuffer const &CommandBuffer) const
{
    std::vector WriteDescriptors {
            VkWriteDescriptorSet {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = VK_NULL_HANDLE,
                    .dstBinding = 0U,
                    .dstArrayElement = 0U,
                    .descriptorCount = 1U,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &GetSceneUniformDescriptor(),
            }
    };

    for (auto &ModelDescriptorIter : m_Allocation.ModelDescriptors)
    {
        WriteDescriptors.push_back(VkWriteDescriptorSet {
                                           .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                           .dstSet = VK_NULL_HANDLE,
                                           .dstBinding = 1U + static_cast<std::uint32_t>(ModelDescriptorIter.first),
                                           .dstArrayElement = 0U,
                                           .descriptorCount = 1U,
                                           .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                           .pBufferInfo = &ModelDescriptorIter.second,
                                   });
    }

    for (auto &TextureDescriptorIter : m_Allocation.TextureDescriptors)
    {
        WriteDescriptors.push_back(VkWriteDescriptorSet {
                                           .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                           .dstSet = VK_NULL_HANDLE,
                                           .dstBinding = 3U + static_cast<std::uint32_t>(TextureDescriptorIter.first),
                                           .dstArrayElement = 0U,
                                           .descriptorCount = 1U,
                                           .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                           .pImageInfo = &TextureDescriptorIter.second,
                                   });
    }

    vkCmdPushDescriptorSetKHR(CommandBuffer,
                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                              GetPipelineLayout(),
                              0U,
                              static_cast<std::uint32_t>(std::size(WriteDescriptors)),
                              std::data(WriteDescriptors));

    bool ActiveVertexBinding = false;
    if (m_Allocation.VertexBufferAllocation.Buffer != VK_NULL_HANDLE)
    {
        vkCmdBindVertexBuffers(CommandBuffer, 0U, 1U, &m_Allocation.VertexBufferAllocation.Buffer, std::data(g_Offsets));
        ActiveVertexBinding = true;
    }

    bool ActiveIndexBinding = false;
    if (m_Allocation.IndexBufferAllocation.Buffer != VK_NULL_HANDLE)
    {
        vkCmdBindIndexBuffer(CommandBuffer, m_Allocation.IndexBufferAllocation.Buffer, 0U, VK_INDEX_TYPE_UINT32);
        ActiveIndexBinding = true;
    }

    if (ActiveVertexBinding && ActiveIndexBinding)
    {
        vkCmdDrawIndexed(CommandBuffer, static_cast<std::uint32_t>(std::size(m_Indices)), 1U, 0U, 0U, 0U);
    }
}
