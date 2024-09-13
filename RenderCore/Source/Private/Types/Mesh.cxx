// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <meshoptimizer.h>

module RenderCore.Types.Mesh;

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

void Mesh::SetupBounds()
{
    for (auto const &VertexIter : m_Vertices)
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

void Mesh::BindBuffers(VkCommandBuffer const &CommandBuffer, std::uint32_t const NumInstances) const
{
    VkBuffer const &AllocationBuffer = GetAllocationBuffer();

    vkCmdBindVertexBuffers(CommandBuffer, 0U, 1U, &AllocationBuffer, &m_VertexOffset);
    vkCmdBindIndexBuffer(CommandBuffer, AllocationBuffer, m_IndexOffset, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(CommandBuffer, static_cast<std::uint32_t>(std::size(m_Indices)), NumInstances, 0U, 0U, 0U);
}
