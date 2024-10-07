// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

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
    std::size_t const IndexCount = std::size(Indices);
    std::size_t CurrentIndex = 0U;

    while (CurrentIndex < IndexCount)
    {
        Meshlet CurrentMeshlet;

        std::size_t PrimitiveCount = 0;

        while (PrimitiveCount < g_MaxMeshletPrimitives && CurrentIndex < IndexCount)
        {
            for (std::size_t TriangleVertexIt = 0; TriangleVertexIt < 3; ++TriangleVertexIt)
            {
                std::uint32_t const IndexIt = Indices.at(CurrentIndex++);
                if (CurrentMeshlet.VertexCount < g_MaxMeshletVertices)
                {
                    CurrentMeshlet.Vertices.at(CurrentMeshlet.VertexCount++) = Vertices.at(IndexIt);
                }

                CurrentMeshlet.Indices.at(CurrentMeshlet.IndexCount++) = IndexIt;
            }

            ++PrimitiveCount;
        }

        m_Meshlets.push_back(std::move(CurrentMeshlet));
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
