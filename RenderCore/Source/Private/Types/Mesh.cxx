// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <meshoptimizer.h>

module RenderCore.Types.Mesh;

import RenderCore.Runtime.Memory;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Utils.Constants;

using namespace RenderCore;

Mesh::Mesh(std::uint32_t const ID, strzilla::string_view const Path)
    : Resource(ID, Path)
{
}

Mesh::Mesh(std::uint32_t const ID, strzilla::string_view const Path, strzilla::string_view const Name)
    : Resource(ID, Path, Name)
{
}

void Mesh::SetupMeshlets(std::vector<Vertex> &&Vertices, std::vector<std::uint32_t> &&Indices)
{
    std::size_t const InitialVertexCount = std::size(Vertices);
    std::size_t const InitialIndexCount = std::size(Indices);

    std::vector<std::uint32_t> Remap(InitialIndexCount);
    std::size_t const          NewVertexCount = meshopt_generateVertexRemap(std::data(Remap),
                                                                           std::data(Indices),
                                                                           InitialIndexCount,
                                                                           std::data(Vertices),
                                                                           InitialVertexCount,
                                                                           sizeof(Vertex));

    std::vector<Vertex>       NewVertices(NewVertexCount);
    std::vector<unsigned int> NewIndices(InitialIndexCount);

    meshopt_remapIndexBuffer(std::data(NewIndices), std::data(Indices), InitialIndexCount, std::data(Remap));
    meshopt_remapVertexBuffer(std::data(NewVertices), std::data(Vertices), InitialVertexCount, sizeof(Vertex), std::data(Remap));

    Vertices     = std::move(NewVertices);
    Indices      = std::move(NewIndices);

    meshopt_optimizeVertexCache(std::data(Indices), std::data(Indices), InitialIndexCount, std::size(Vertices));

    meshopt_optimizeOverdraw(std::data(Indices),
                             std::data(Indices),
                             InitialIndexCount,
                             &Vertices.at(0).Position.x,
                             std::size(Vertices),
                             sizeof(Vertex),
                             1.05f);

    meshopt_optimizeVertexFetch(std::data(Vertices),
                                std::data(Indices),
                                InitialIndexCount,
                                std::data(Vertices),
                                std::size(Vertices),
                                sizeof(Vertex));

    std::size_t const PostOptVertexCount = std::size(Vertices);
    std::size_t const PostOptIndexCount = std::size(Indices);

    std::size_t const MaxMeshlets = meshopt_buildMeshletsBound(PostOptIndexCount, g_MaxMeshletVertices, g_MaxMeshletPrimitives);

    std::vector<meshopt_Meshlet> OptimizerMeshlets(MaxMeshlets);
    std::vector<std::uint32_t> MeshletVertices(MaxMeshlets * g_MaxMeshletVertices);
    std::vector<std::uint8_t> MeshletTriangles(MaxMeshlets * g_MaxMeshletPrimitives * 3);

    std::size_t const NumMeshlets = meshopt_buildMeshlets(std::data(OptimizerMeshlets),
                                                          std::data(MeshletVertices),
                                                          std::data(MeshletTriangles),
                                                          std::data(Indices),
                                                          PostOptIndexCount,
                                                          &Vertices.at(0).Position.x,
                                                          PostOptVertexCount,
                                                          sizeof(Vertex),
                                                          g_MaxMeshletVertices,
                                                          g_MaxMeshletPrimitives,
                                                          0.F);

    auto const &[MeshletVertexOffset, MeshletTriangleOffset, MeshletVertexCount, MeshletTriangleCount] = OptimizerMeshlets.at(NumMeshlets - 1U);

    MeshletVertices.resize(MeshletVertexOffset + MeshletVertexCount);
    MeshletTriangles.resize(MeshletTriangleOffset + (MeshletTriangleCount * 3 + 3 & ~3));
    OptimizerMeshlets.resize(NumMeshlets);

    meshopt_optimizeMeshlet(&MeshletVertices.at(MeshletVertexOffset), &MeshletTriangles.at(MeshletTriangleOffset), MeshletTriangleCount, MeshletVertexCount);

    m_Meshlets.resize(NumMeshlets);

    for (std::size_t MeshletIt = 0; MeshletIt < NumMeshlets; ++MeshletIt)
    {
        Meshlet NewMeshlet;

        auto const &[MeshletVertexOffset,
                     MeshletTriangleOffset,
                     MeshletVertexCount,
                     MeshletTriangleCount] = OptimizerMeshlets[MeshletIt];

        for (std::size_t VertexIt = 0U; VertexIt < MeshletVertexCount; ++VertexIt)
        {
            NewMeshlet.Vertices.at(VertexIt) = Vertices.at(MeshletVertices.at(MeshletVertexOffset + VertexIt));
        }

        for (std::size_t TriangleIt = 0U; TriangleIt < MeshletTriangleCount * 3U; ++TriangleIt)
        {
            NewMeshlet.Indices.at(TriangleIt) = MeshletTriangles.at(TriangleIt + MeshletTriangleOffset);
            NewMeshlet.Indices.at(TriangleIt) = MeshletTriangles.at(TriangleIt + MeshletTriangleOffset + 1U);
            NewMeshlet.Indices.at(TriangleIt) = MeshletTriangles.at(TriangleIt + MeshletTriangleOffset + 2U);
        }

        NewMeshlet.VertexCount = MeshletVertexCount;
        NewMeshlet.IndexCount = MeshletTriangleCount * 3;

        m_Meshlets.at(MeshletIt) = NewMeshlet;
    }
}

void Mesh::UpdateUniformBuffers(char * const OwningData) const
{
    if (!OwningData)
    {
        return;
    }

    if (IsRenderDirty())
    {
        constexpr auto ModelUBOSize    = sizeof(ModelUniformData);
        constexpr auto MaterialUBOSize = sizeof(MaterialData);
        std::memcpy(OwningData + ModelUBOSize, &GetMaterialData(), MaterialUBOSize);

        SetRenderDirty(false);
    }
}
