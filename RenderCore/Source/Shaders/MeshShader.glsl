#version 460

#extension GL_EXT_mesh_shader : require
#extension GL_EXT_control_flow_attributes : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require

#include "Types.h"

layout(local_size_x = g_NumTasks) in;

layout(max_vertices = g_NumVertices, max_primitives = g_NumPrimitives) out;
layout(triangles) out;

layout(std140, set = 1, binding = 0) uniform ModelUniformData
{
    ModelUBO Data;
} ModelData;

layout(std140, set = 3, binding = 0) buffer MeshletBuffer
{
    Meshlet Buffer[];
} MeshletData;

layout(location = 0) out FragSharedData
{
    FragmentData Data;
} FragData[64];

void main()
{
    uint MeshletIndex = gl_WorkGroupID.x;
    if (MeshletIndex >= ModelData.Data.NumMeshlets) return;

    Meshlet CurrentMeshlet = MeshletData.Buffer[MeshletIndex];

    uint NumVertices = CurrentMeshlet.NumVertices;
    uint NumIndices = CurrentMeshlet.NumIndices;

    SetMeshOutputsEXT(NumVertices, NumIndices);

    [[unroll]] for (uint Iterator = 0; Iterator < g_MaxVertexIterations; Iterator++)
    {
        vec3 Position = GetPosition(CurrentMeshlet, Iterator);
        vec3 Normal = GetNormal(CurrentMeshlet, Iterator);
        vec2 UV = GetUV(CurrentMeshlet, Iterator);

        vec4 ClipPosition = ModelData.Data.ProjectionView * vec4(Position, 1.0);

        FragData[Iterator].Data.FragPosition = ClipPosition;
        FragData[Iterator].Data.FragNormal = Normal;
        FragData[Iterator].Data.FragUV = UV;

        FragData[Iterator].Data.FragColor = vec4(MeshletColors[MeshletIndex % g_MaxColors], 1.0);

        gl_MeshVerticesEXT[Iterator].gl_Position = ClipPosition;
    }

    [[unroll]] for (uint Iterator = 0; Iterator < g_MaxIndexIterations / 3; Iterator++)
    {
        uint Index0 = CurrentMeshlet.Indices[Iterator * 3];
        uint Index1 = CurrentMeshlet.Indices[Iterator * 3 + 1];
        uint Index2 = CurrentMeshlet.Indices[Iterator * 3 + 2];

        gl_PrimitiveTriangleIndicesEXT[Iterator] = uvec3(Index0, Index1, Index2);
    }
}
