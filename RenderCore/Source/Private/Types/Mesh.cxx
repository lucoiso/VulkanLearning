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

void Mesh::SetupVertices(std::vector<Vertex> const &Value)
{
    std::size_t const NumRequested = std::size(Value);
    std::size_t const NumMeshlets = (NumRequested + g_MaxMeshletVertices - 1) / g_MaxMeshletVertices;

    m_Meshlets.resize(NumMeshlets);

    for (std::size_t Iterator = 0; Iterator < NumMeshlets; ++Iterator)
    {
        std::size_t const StartIndex = Iterator * g_MaxMeshletVertices;
        std::size_t const EndIndex = std::min(StartIndex + g_MaxMeshletVertices, NumRequested);
        std::size_t const VertexCount = EndIndex - StartIndex;

        std::copy(std::cbegin(Value) + static_cast<std::ptrdiff_t>(StartIndex),
                  std::cbegin(Value) + static_cast<std::ptrdiff_t>(EndIndex),
                  std::data(m_Meshlets.at(Iterator).Vertices));

        m_Meshlets.at(Iterator).VertexCount = static_cast<std::uint32_t>(VertexCount);
    }
}

void Mesh::SetupIndices(std::vector<std::uint32_t> const &Value)
{
    std::size_t const CurrentNum = std::size(m_Meshlets);
    std::size_t const NumRequested = std::size(Value);
    constexpr std::uint8_t MaxIndices = g_MaxMeshletPrimitives * 3U;
    std::size_t const NumMeshlets = (NumRequested + MaxIndices - 1) / MaxIndices;

    m_Meshlets.resize(std::max(CurrentNum, NumMeshlets));

    for (std::size_t Iterator = 0; Iterator < NumMeshlets; ++Iterator)
    {
        std::size_t const StartIndex = Iterator * MaxIndices;
        std::size_t const EndIndex = std::min(StartIndex + MaxIndices, NumRequested);
        std::size_t const IndexCount = EndIndex - StartIndex;

        std::copy(std::cbegin(Value) + static_cast<std::ptrdiff_t>(StartIndex),
                  std::cbegin(Value) + static_cast<std::ptrdiff_t>(EndIndex),
                  std::data(m_Meshlets.at(Iterator).Indices));

        m_Meshlets.at(Iterator).IndexCount = static_cast<std::uint32_t>(IndexCount);
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
