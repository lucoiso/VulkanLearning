// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <Volk/volk.h>
#include <glm/ext.hpp>
#include <string>

module RenderCore.Types.Object;

import RenderCore.Renderer;
import RenderCore.Runtime.Buffer;
import RenderCore.Runtime.Pipeline;
import RenderCore.Utils.Constants;

using namespace RenderCore;

Object::Object(std::uint32_t const ID, std::string_view const Path)
    : m_ID(ID)
    , m_Path(Path)
    , m_Name(m_Path.substr(m_Path.find_last_of('/') + 1, m_Path.find_last_of('.') - m_Path.find_last_of('/') - 1))
{
}

Object::Object(std::uint32_t const ID, std::string_view const Path, std::string_view const Name) : m_ID(ID), m_Path(Path), m_Name(Name)
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

Vector Object::GetPosition() const
{
    return m_Transform.Position;
}

void Object::SetPosition(Vector const &Position)
{
    m_Transform.Position = Position;
}

Rotator Object::GetRotation() const
{
    return m_Transform.Rotation;
}

void Object::SetRotation(Rotator const &Rotation)
{
    m_Transform.Rotation = Rotation;
}

Vector Object::GetScale() const
{
    return m_Transform.Scale;
}

void Object::SetScale(Vector const &Scale)
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
    if (!m_Allocation.UniformBufferAllocation.MappedData)
    {
        return;
    }

    glm::mat4 const UpdatedUBO {GetMatrix()};
    std::memcpy(m_Allocation.UniformBufferAllocation.MappedData, &UpdatedUBO, sizeof(glm::mat4));
}

void Object::DrawObject(VkCommandBuffer const &CommandBuffer) const
{
    std::vector WriteDescriptors {VkWriteDescriptorSet {
                                      .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                      .dstSet          = VK_NULL_HANDLE,
                                      .dstBinding      = 0U,
                                      .dstArrayElement = 0U,
                                      .descriptorCount = 1U,
                                      .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                      .pBufferInfo     = &GetSceneUniformDescriptor(),
                                  },
                                  VkWriteDescriptorSet {
                                      .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                      .dstSet          = VK_NULL_HANDLE,
                                      .dstBinding      = 1U,
                                      .dstArrayElement = 0U,
                                      .descriptorCount = static_cast<std::uint32_t>(std::size(m_Allocation.ModelDescriptors)),
                                      .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                      .pBufferInfo     = std::data(m_Allocation.ModelDescriptors),
                                  }};

    for (auto &TextureDescriptorIter : m_Allocation.TextureDescriptors)
    {
        WriteDescriptors.push_back(VkWriteDescriptorSet {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = VK_NULL_HANDLE,
            .dstBinding      = 2U + static_cast<std::uint32_t>(TextureDescriptorIter.first),
            .dstArrayElement = 0U,
            .descriptorCount = 1U,
            .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo      = &TextureDescriptorIter.second,
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
