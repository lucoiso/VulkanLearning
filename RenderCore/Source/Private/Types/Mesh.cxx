// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <meshoptimizer.h>

module RenderCore.Types.Mesh;

import RenderCore.Runtime.Memory;
import RenderCore.Runtime.Pipeline;
import RenderCore.Runtime.Scene;
import RenderCore.Runtime.Device;
import RenderCore.Types.Material;
import RenderCore.Types.Texture;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Utils.Helpers;
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
    std::vector<std::uint8_t> MeshletTriangles(MaxMeshlets * g_MaxMeshletIndices);

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

    std::size_t const LasNumVertices = MeshletVertexOffset + MeshletVertexCount;
    MeshletVertices.resize(LasNumVertices);

    std::size_t const LastNumTriangles = MeshletTriangleOffset + (MeshletTriangleCount * 3 + 3 & ~3);
    MeshletTriangles.resize(LastNumTriangles);

    OptimizerMeshlets.resize(NumMeshlets);

    meshopt_optimizeMeshlet(&MeshletVertices.at(MeshletVertexOffset), &MeshletTriangles.at(MeshletTriangleOffset), MeshletTriangleCount, MeshletVertexCount);

    m_Meshlets       .resize(NumMeshlets);
    m_Indices        .resize(LastNumTriangles * 3U);
    m_VertexUVs      .resize(LasNumVertices);
    m_VertexPositions.resize(LasNumVertices);
    m_VertexNormals  .resize(LasNumVertices);
    m_VertexColors   .resize(LasNumVertices);
    m_VertexJoints   .resize(LasNumVertices);
    m_VertexWeights  .resize(LasNumVertices);
    m_VertexTangents .resize(LasNumVertices);

    for (std::size_t MeshletIt = 0; MeshletIt < NumMeshlets; ++MeshletIt)
    {
        auto const &[LocalMeshletVertexOffset,
                     LocalMeshletTriangleOffset,
                     LocalMeshletVertexCount,
                     LocalMeshletTriangleCount] = OptimizerMeshlets[MeshletIt];

        Meshlet &NewMeshlet = m_Meshlets.at(MeshletIt);

        NewMeshlet.VertexCount = LocalMeshletVertexCount;
        NewMeshlet.IndexCount = LocalMeshletTriangleCount * 3U;
        NewMeshlet.VertexOffset = GetVertexUVsLocalOffset() + MeshletVertexOffset;
        NewMeshlet.IndexOffset = GetIndicesLocalOffset() + MeshletTriangleOffset;

        for (std::size_t VertexIt = 0U; VertexIt < LocalMeshletVertexCount; ++VertexIt)
        {
            Vertex const& OptVertex = Vertices.at(MeshletVertices.at(LocalMeshletVertexOffset + VertexIt));

            m_VertexUVs.at(VertexIt) = OptVertex.TextureCoordinate;
            m_VertexPositions.at(VertexIt) = OptVertex.Position;
            m_VertexNormals.at(VertexIt) = OptVertex.Normal;
            m_VertexColors.at(VertexIt) = OptVertex.Color;
            m_VertexJoints.at(VertexIt) = OptVertex.Joint;
            m_VertexWeights.at(VertexIt) = OptVertex.Weight;
            m_VertexTangents.at(VertexIt) = OptVertex.Tangent;
        }

        for (std::size_t IndexIt = 0U; IndexIt < LocalMeshletTriangleCount * 3U; ++IndexIt)
        {
            m_Indices.at(IndexIt) = MeshletTriangles.at(IndexIt + LocalMeshletTriangleOffset);
        }

        m_Meshlets.at(MeshletIt) = NewMeshlet;
    }
}

void RenderCore::Mesh::SetupUniformDescriptor()
{
    m_UniformBufferInfo = GetAllocationBufferDescriptor(GetUniformOffset(), sizeof(ModelUniformData));
    m_MappedData = GetAllocationMappedData();
}

void Mesh::UpdateUniformBuffers() const
{
    if (!m_MappedData)
    {
        return;
    }

    if (IsRenderDirty())
    {
        ModelUniformData const UpdatedModelUBO{.ProjectionView = GetCamera().GetProjectionMatrix(),
                                               .Model = m_Transform.GetMatrix()};

        auto const UniformData = static_cast<char*>(m_MappedData) + GetUniformOffset();
        std::memcpy(UniformData, &UpdatedModelUBO, sizeof(ModelUniformData));

        auto const MaterialData = static_cast<char*>(m_MappedData) + GetMaterialOffset();
        std::memcpy(MaterialData, &GetMaterialData(), sizeof(MaterialData));

        SetRenderDirty(false);
    }
}

void RenderCore::Mesh::DrawObject(VkCommandBuffer const& CommandBuffer, VkPipelineLayout const& PipelineLayout, std::uint32_t const ObjectIndex) const
{
    auto const &[SceneData,
                 ModelData,
                 MaterialData,
                 MeshletData,
                 IndexData,
                 VertexUVData,
                 VertexPositionData,
                 VertexNormalData,
                 VertexColorData,
                 VertexJointData,
                 VertexWeightData,
                 VertexTangentData,
                 TextureData] = GetPipelineDescriptorData();

    std::array const BufferBindingInfos {
            VkDescriptorBufferBindingInfoEXT
            {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                    .address = SceneData.BufferDeviceAddress.deviceAddress,
                    .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
            },
            VkDescriptorBufferBindingInfoEXT {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                    .address = ModelData.BufferDeviceAddress.deviceAddress,
                    .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
            },
            VkDescriptorBufferBindingInfoEXT {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                    .address = MaterialData.BufferDeviceAddress.deviceAddress,
                    .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
            },
            VkDescriptorBufferBindingInfoEXT {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                    .address = MeshletData.BufferDeviceAddress.deviceAddress,
                    .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
            },
            VkDescriptorBufferBindingInfoEXT {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                    .address = IndexData.BufferDeviceAddress.deviceAddress,
                    .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
            },
            VkDescriptorBufferBindingInfoEXT {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                    .address = VertexUVData.BufferDeviceAddress.deviceAddress,
                    .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
            },
            VkDescriptorBufferBindingInfoEXT {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                    .address = VertexPositionData.BufferDeviceAddress.deviceAddress,
                    .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
            },
            VkDescriptorBufferBindingInfoEXT {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                    .address = VertexNormalData.BufferDeviceAddress.deviceAddress,
                    .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
            },
            VkDescriptorBufferBindingInfoEXT {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                    .address = VertexColorData.BufferDeviceAddress.deviceAddress,
                    .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
            },
            VkDescriptorBufferBindingInfoEXT {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                    .address = VertexJointData.BufferDeviceAddress.deviceAddress,
                    .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
            },
            VkDescriptorBufferBindingInfoEXT {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                    .address = VertexWeightData.BufferDeviceAddress.deviceAddress,
                    .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
            },
            VkDescriptorBufferBindingInfoEXT {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                    .address = VertexTangentData.BufferDeviceAddress.deviceAddress,
                    .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
            },
            VkDescriptorBufferBindingInfoEXT {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                    .address = TextureData.BufferDeviceAddress.deviceAddress,
                    .usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
            }
    };

    vkCmdBindDescriptorBuffersEXT(CommandBuffer, static_cast<std::uint32_t>(std::size(BufferBindingInfos)), std::data(BufferBindingInfos));

    std::vector<std::uint32_t> BufferIndices(std::size(BufferBindingInfos));
    for (std::size_t Iterator = 0U; Iterator < std::size(BufferIndices); ++Iterator)
    {
        BufferIndices.at(Iterator) = Iterator;
    }

    constexpr auto NumTextures = static_cast<std::uint8_t>(TextureType::Count);

    std::array const BufferOffsets {
            SceneData.LayoutOffset,
            ObjectIndex * ModelData.LayoutSize                 + ModelData.LayoutOffset,
            ObjectIndex * MaterialData.LayoutSize              + MaterialData.LayoutOffset,
            ObjectIndex * MeshletData.LayoutSize               + MeshletData.LayoutOffset,
            ObjectIndex * IndexData.LayoutSize                 + IndexData.LayoutOffset,
            ObjectIndex * VertexUVData.LayoutSize              + VertexUVData.LayoutOffset,
            ObjectIndex * VertexPositionData.LayoutSize        + VertexPositionData.LayoutOffset,
            ObjectIndex * VertexNormalData.LayoutSize          + VertexNormalData.LayoutOffset,
            ObjectIndex * VertexColorData.LayoutSize           + VertexColorData.LayoutOffset,
            ObjectIndex * VertexJointData.LayoutSize           + VertexJointData.LayoutOffset,
            ObjectIndex * VertexWeightData.LayoutSize          + VertexWeightData.LayoutOffset,
            ObjectIndex * VertexTangentData.LayoutSize         + VertexTangentData.LayoutOffset,
            ObjectIndex * NumTextures * TextureData.LayoutSize + TextureData.LayoutOffset
    };

    vkCmdSetDescriptorBufferOffsetsEXT(CommandBuffer,
                                       VK_PIPELINE_BIND_POINT_GRAPHICS,
                                       PipelineLayout,
                                       0U,
                                       static_cast<std::uint32_t>(std::size(BufferBindingInfos)),
                                       std::data(BufferIndices),
                                       std::data(BufferOffsets));

    std::uint32_t const NumMeshlets = GetNumMeshlets();
    std::uint32_t const NumTasks = std::trunc(NumMeshlets + g_MaxMeshTasks - 1U) / g_MaxMeshTasks;

    if (vkCmdDrawMeshTasksNV != nullptr)
    {
        vkCmdDrawMeshTasksNV(CommandBuffer, NumMeshlets, 0U);
    }
    else
    {
        vkCmdDrawMeshTasksEXT(CommandBuffer, NumMeshlets, 1U, 1U);
    }
}
