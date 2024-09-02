// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <meshoptimizer.h>

module RenderCore.Types.Mesh;

import RenderCore.Runtime.Scene;
import RenderCore.Runtime.Memory;

using namespace RenderCore;

Mesh::Mesh(std::uint32_t const ID, strzilla::string_view const Path)
    : Resource(ID, Path)
{
}

Mesh::Mesh(std::uint32_t const ID, strzilla::string_view const Path, strzilla::string_view const Name)
    : Resource(ID, Path, Name)
{
}

void Mesh::Optimize()
{
    std::size_t const IndexCount  = m_NumTriangles * 3;
    std::size_t const VertexCount = std::size(m_Vertices);

    std::vector<unsigned int> Remap(IndexCount);
    std::size_t const         NewVertexCount = meshopt_generateVertexRemap(std::data(Remap),
                                                                           std::data(m_Indices),
                                                                           IndexCount,
                                                                           std::data(m_Vertices),
                                                                           VertexCount,
                                                                           sizeof(Vertex));

    std::vector<Vertex>       NewVertices(NewVertexCount);
    std::vector<unsigned int> NewIndices(IndexCount);

    meshopt_remapIndexBuffer(std::data(NewIndices), std::data(m_Indices), IndexCount, std::data(Remap));
    meshopt_remapVertexBuffer(std::data(NewVertices), std::data(m_Vertices), VertexCount, sizeof(Vertex), std::data(Remap));

    m_Vertices     = std::move(NewVertices);
    m_Indices      = std::move(NewIndices);
    m_NumTriangles = static_cast<std::uint32_t>(std::size(m_Indices) / 3U);

    meshopt_optimizeVertexCache(std::data(m_Indices), std::data(m_Indices), IndexCount, std::size(m_Vertices));

    meshopt_optimizeOverdraw(std::data(m_Indices),
                             std::data(m_Indices),
                             IndexCount,
                             &m_Vertices[0].Position.x,
                             std::size(m_Vertices),
                             sizeof(Vertex),
                             1.05f);

    meshopt_optimizeVertexFetch(std::data(m_Vertices),
                                std::data(m_Indices),
                                IndexCount,
                                std::data(m_Vertices),
                                std::size(m_Vertices),
                                sizeof(Vertex));
}

Transform const &Mesh::GetTransform() const
{
    return m_Transform;
}

void Mesh::SetTransform(Transform const &Transform)
{
    m_Transform = Transform;
}

glm::vec3 Mesh::GetCenter() const
{
    return (m_Bounds.Min + m_Bounds.Max) / 2.0f;
}

float Mesh::GetSize() const
{
    return distance(m_Bounds.Min, m_Bounds.Max);
}

Bounds const &Mesh::GetBounds() const
{
    return m_Bounds;
}

void Mesh::SetupBounds()
{
    EASY_FUNCTION(profiler::colors::Red50);

    for (const auto &VertexIter : m_Vertices)
    {
        glm::vec4 const TransformedVertex = glm::vec4(VertexIter.Position, 1.0f) * m_Transform.GetMatrix();

        m_Bounds.Min.x = std::min(m_Bounds.Min.x, TransformedVertex.x);
        m_Bounds.Min.y = std::min(m_Bounds.Min.y, TransformedVertex.y);
        m_Bounds.Min.z = std::min(m_Bounds.Min.z, TransformedVertex.z);

        m_Bounds.Max.x = std::max(m_Bounds.Max.x, TransformedVertex.x);
        m_Bounds.Max.y = std::max(m_Bounds.Max.y, TransformedVertex.y);
        m_Bounds.Max.z = std::max(m_Bounds.Max.z, TransformedVertex.z);
    }
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
    m_Indices      = Indices;
    m_NumTriangles = static_cast<std::uint32_t>(std::size(m_Indices) / 3U);
}

std::uint32_t Mesh::GetNumTriangles() const
{
    return m_NumTriangles;
}

std::uint32_t Mesh::GetNumVertices() const
{
    return static_cast<std::uint32_t>(std::size(m_Vertices));
}

std::uint32_t Mesh::GetNumIndices() const
{
    return static_cast<std::uint32_t>(std::size(m_Indices));
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

void Mesh::BindBuffers(VkCommandBuffer const &CommandBuffer, std::uint32_t const NumInstances) const
{
    EASY_FUNCTION(profiler::colors::Red50);

    EASY_VALUE("ID",        GetID());
    EASY_VALUE("Triangles", GetNumTriangles());
    EASY_VALUE("Indices",   GetNumVertices());
    EASY_VALUE("Vertices",  GetNumIndices());

    VkBuffer const &AllocationBuffer = GetAllocationBuffer();

    vkCmdBindVertexBuffers(CommandBuffer, 0U, 1U, &AllocationBuffer, &m_VertexOffset);
    vkCmdBindIndexBuffer(CommandBuffer, AllocationBuffer, m_IndexOffset, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(CommandBuffer, static_cast<std::uint32_t>(std::size(m_Indices)), NumInstances, 0U, 0U, 0U);
}
