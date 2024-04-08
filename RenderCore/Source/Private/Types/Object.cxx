// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <Volk/volk.h>
#include <glm/ext.hpp>
#include <string>
#include <array>

module RenderCore.Types.Object;

import RenderCore.Renderer;
import RenderCore.Runtime.Memory;
import RenderCore.Runtime.Pipeline;
import RenderCore.Runtime.Scene;
import RenderCore.Runtime.Device;
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

Transform const &Object::GetTransform() const
{
    return m_Transform;
}

void Object::SetTransform(Transform const &Value)
{
    if (m_Transform != Value)
    {
        m_Transform          = Value;
        m_IsModelRenderDirty = true;
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
        m_IsModelRenderDirty = true;
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
        m_IsModelRenderDirty = true;
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
        m_IsModelRenderDirty = true;
    }
}

glm::mat4 Object::GetMatrix() const
{
    return m_Transform.GetMatrix();
}

void Object::SetMatrix(glm::mat4 const &Value)
{
    m_Transform.SetMatrix(Value);
    m_IsModelRenderDirty = true;
}

MaterialData const &Object::GetMaterialData() const
{
    return m_MaterialData;
}

void Object::SetMaterialData(MaterialData const &Value)
{
    if (m_MaterialData != Value)
    {
        m_MaterialData          = Value;
        m_IsMaterialRenderDirty = true;
    }
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
    m_Vertices           = Value;
    m_IsModelRenderDirty = true;
}

std::vector<Vertex> const &Object::GetVertices() const
{
    return m_Vertices;
}

void Object::SetIndexBuffer(std::vector<std::uint32_t> const &Value)
{
    m_Indices            = Value;
    m_IsModelRenderDirty = true;
}

std::vector<std::uint32_t> const &Object::GetIndices() const
{
    return m_Indices;
}

std::uint32_t Object::GetNumVertices() const
{
    return static_cast<std::uint32_t>(std::size(m_Vertices));
}

std::uint32_t Object::GetNumIndices() const
{
    return static_cast<std::uint32_t>(std::size(m_Indices));
}

std::uint32_t Object::GetNumTriangles() const
{
    return GetNumIndices() / 3U;
}

std::size_t Object::GetVertexBufferSize() const
{
    return GetNumVertices() * sizeof(Vertex);
}

std::size_t Object::GetIndexBufferSize() const
{
    return GetNumIndices() * sizeof(std::uint32_t);
}

std::size_t Object::GetModelUniformBufferSize() const
{
    return sizeof(ModelUniformData);
}

std::size_t Object::GetMaterialUniformBufferSize() const
{
    return sizeof(MaterialUniformData);
}

std::size_t Object::GetAllocationSize() const
{
    std::size_t Output = GetVertexBufferSize() + GetIndexBufferSize() + GetModelUniformBufferSize() + GetMaterialUniformBufferSize();
    if (GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment > 0U)
    {
        Output = Output + GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment - 1U & ~(
                     GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment - 1U);
    }

    return Output;
}

std::size_t Object::GetVertexBufferStart() const
{
    return 0U;
}

std::size_t Object::GetVertexBufferEnd() const
{
    return GetVertexBufferStart() + GetVertexBufferSize();
}

std::size_t Object::GetIndexBufferStart() const
{
    return GetVertexBufferEnd();
}

std::size_t Object::GetIndexBufferEnd() const
{
    return GetVertexBufferEnd() + GetIndexBufferSize();
}

std::size_t Object::GetModelUniformStart() const
{
    std::size_t Output = GetIndexBufferEnd();
    if (GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment > 0U)
    {
        Output = Output + GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment - 1U & ~(
                     GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment - 1U);
    }

    return Output;
}

std::size_t Object::GetModelUniformEnd() const
{
    return GetIndexBufferEnd() + GetModelUniformBufferSize();
}

std::size_t Object::GetMaterialUniformStart() const
{
    std::size_t Output = GetModelUniformEnd();
    if (GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment > 0U)
    {
        Output = Output + GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment - 1U & ~(
                     GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment - 1U);
    }

    return Output;
}

std::size_t Object::GetMaterialUniformEnd() const
{
    return GetModelUniformEnd() + GetMaterialUniformBufferSize();
}

void Object::UpdateUniformBuffers() const
{
    if (m_IsModelRenderDirty && m_Allocation.BufferAllocation.MappedData)
    {
        ModelUniformData const UpdatedModelUBO { .Model = m_Transform.GetMatrix() };
        std::memcpy(static_cast<char *>(m_Allocation.BufferAllocation.MappedData) + GetModelUniformStart(),
                    &UpdatedModelUBO,
                    GetModelUniformBufferSize());
        m_IsModelRenderDirty = false;
    }

    if (m_IsMaterialRenderDirty && m_Allocation.BufferAllocation.MappedData)
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

        std::memcpy(static_cast<char *>(m_Allocation.BufferAllocation.MappedData) + GetMaterialUniformStart(),
                    &UpdatedMaterialUBO,
                    GetMaterialUniformBufferSize());
        m_IsMaterialRenderDirty = false;
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
                    .pBufferInfo = &GetCamera().GetUniformDescriptor(),
            },
            VkWriteDescriptorSet {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = VK_NULL_HANDLE,
                    .dstBinding = 1U,
                    .dstArrayElement = 0U,
                    .descriptorCount = 1U,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &GetIllumination().GetUniformDescriptor(),
            }
    };

    for (auto &ModelDescriptorIter : m_Allocation.ModelDescriptors)
    {
        WriteDescriptors.push_back(VkWriteDescriptorSet {
                                           .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                           .dstSet = VK_NULL_HANDLE,
                                           .dstBinding = 2U + static_cast<std::uint32_t>(ModelDescriptorIter.first),
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
                                           .dstBinding = 4U + static_cast<std::uint32_t>(TextureDescriptorIter.first),
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

    if (m_Allocation.BufferAllocation.IsValid())
    {
        std::array<VkDeviceSize, 1U> const VertexOffsets { GetVertexBufferStart() };
        vkCmdBindVertexBuffers(CommandBuffer, 0U, 1U, &m_Allocation.BufferAllocation.Buffer, std::data(VertexOffsets));
        vkCmdBindIndexBuffer(CommandBuffer, m_Allocation.BufferAllocation.Buffer, GetIndexBufferStart(), VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(CommandBuffer, static_cast<std::uint32_t>(std::size(m_Indices)), 1U, 0U, 0U, 0U);
    }
}
