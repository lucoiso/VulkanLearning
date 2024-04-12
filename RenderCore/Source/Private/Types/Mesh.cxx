// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <string_view>
#include <Volk/volk.h>

module RenderCore.Types.Mesh;

import RenderCore.Runtime.Scene;
import RenderCore.Runtime.Memory;

using namespace RenderCore;

Mesh::Mesh(std::uint32_t const ID, std::string_view const Path)
    : Resource(ID, Path)
{
}

Mesh::Mesh(std::uint32_t const ID, std::string_view const Path, std::string_view const Name)
    : Resource(ID, Path, Name)
{
}

Transform const &Mesh::GetTransform() const
{
    return m_Transform;
}

void Mesh::SetTransform(Transform const &Transform)
{
    m_Transform = Transform;
}

std::vector<Vertex> const &Mesh::GetVertices() const
{
    return m_Vertices;
}

void Mesh::SetVertices(std::vector<Vertex> const &Vertices)
{
    m_Vertices = Vertices;
}

std::vector<std::uint32_t> const &Mesh::GetIndices() const
{
    return m_Indices;
}

void Mesh::SetIndices(std::vector<std::uint32_t> const &Indices)
{
    m_Indices = Indices;
}

std::uint32_t Mesh::GetNumTriangles() const
{
    return static_cast<std::uint32_t>(std::size(m_Indices) / 3U);
}

VkDeviceSize Mesh::GetVertexOffset() const
{
    return m_VertexOffset;
}

void Mesh::SetVertexOffset(VkDeviceSize const &VertexOffset)
{
    m_VertexOffset = VertexOffset;
}

VkDeviceSize Mesh::GetIndexOffset() const
{
    return m_IndexOffset;
}

void Mesh::SetIndexOffset(VkDeviceSize const &IndexOffset)
{
    m_IndexOffset = IndexOffset;
}

MaterialData const &Mesh::GetMaterialData() const
{
    return m_MaterialData;
}

void Mesh::SetMaterialData(MaterialData const &MaterialData)
{
    m_MaterialData = MaterialData;
}

std::vector<std::shared_ptr<Texture>> const &Mesh::GetTextures() const
{
    return m_Textures;
}

void Mesh::SetTextures(std::vector<std::shared_ptr<Texture>> const &Textures)
{
    m_Textures = Textures;

    for (auto const &Texture : m_Textures)
    {
        Texture->SetupTexture();
    }
}

std::vector<VkWriteDescriptorSet> Mesh::GetWriteDescriptorSet() const
{
    std::vector<VkWriteDescriptorSet> Output {};
    Output.reserve(std::size(m_Textures));

    for (auto const &Texture : m_Textures)
    {
        auto const WriteDescriptorSet = Texture->GetWriteDescriptorSet();
        Output.insert(std::end(Output), std::begin(WriteDescriptorSet), std::end(WriteDescriptorSet));
    }

    return Output;
}

void Mesh::BindBuffers(VkCommandBuffer const &CommandBuffer) const
{
    VkBuffer const &AllocationBuffer = GetAllocationBuffer(GetBufferIndex());

    vkCmdBindVertexBuffers(CommandBuffer, 0U, 1U, &AllocationBuffer, &m_VertexOffset);
    vkCmdBindIndexBuffer(CommandBuffer, AllocationBuffer, m_IndexOffset, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(CommandBuffer, static_cast<std::uint32_t>(std::size(m_Indices)), 1U, 0U, 0U, 0U);
}
