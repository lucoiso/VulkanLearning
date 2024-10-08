#version 460

#extension GL_EXT_control_flow_attributes : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require

#include "Types.h"

#if g_UseExternalMeshShader
#extension GL_EXT_mesh_shader : require
#else
#extension GL_NV_mesh_shader : require
#endif // g_UseExternalMeshShader

layout(local_size_x = g_NumTasks) in;

layout(max_vertices = g_NumVertices, max_primitives = g_NumPrimitives) out;
layout(triangles) out;

layout(std140, set = 1, binding = 0) uniform ModelUniformData
{
    ModelUBO Data;
} ModelData;

layout(std430, set = 3, binding = 0) buffer MeshletBuffer
{
    Meshlet Buffer[];
} MeshletData;

layout(location = 0) out FragSharedData
{
    FragmentData Data;
} FragData[g_NumVertices];

void main()
{
    uint MeshletIndex = gl_WorkGroupID.x;
    if (MeshletIndex >= ModelData.Data.NumMeshlets) return;

    Meshlet CurrentMeshlet = MeshletData.Buffer[MeshletIndex];

    uint NumVertices = CurrentMeshlet.NumVertices;
    uint NumIndices = CurrentMeshlet.NumIndices;
    
#if g_UseExternalMeshShader
    SetMeshOutputsEXT(NumVertices, NumIndices);
#else
    gl_PrimitiveCountNV = NumIndices / 3;
#endif // g_UseExternalMeshShader

    [[unroll]] for (uint Iterator = 0; Iterator < NumVertices; Iterator++)
    {
        vec3 Position = GetPosition(CurrentMeshlet, Iterator);        
        vec4 WorldPos = ModelData.Data.ModelView * vec4(Position, 1.0);
        vec4 ViewPos = ModelData.Data.ProjectionView * WorldPos;
        
#if g_UseExternalMeshShader
        gl_MeshVerticesEXT[Iterator].gl_Position = ViewPos;
#else
        gl_MeshVerticesNV[Iterator].gl_Position = ViewPos;
#endif // g_UseExternalMeshShader

        vec3 Normal = normalize(mat3(ModelData.Data.ModelView) * GetNormal(CurrentMeshlet, Iterator));
        vec2 UV = GetUV(CurrentMeshlet, Iterator);

        FragData[Iterator].Data.FragView = ViewPos.xyz;
        FragData[Iterator].Data.FragNormal = Normal;
        FragData[Iterator].Data.FragUV = UV;

        FragData[Iterator].Data.FragColor = vec4(MeshletColors[MeshletIndex % g_MaxColors], 1.0);
    }

    [[unroll]] for (uint Iterator = 0; Iterator < NumIndices / 3; Iterator += 3)
    {
        uint Index0 = CurrentMeshlet.Indices[Iterator];
        uint Index1 = CurrentMeshlet.Indices[Iterator + 1];
        uint Index2 = CurrentMeshlet.Indices[Iterator + 2];
        
#if g_UseExternalMeshShader
        gl_PrimitiveTriangleIndicesEXT[Iterator] = uvec3(Index0, Index1, Index2);
#else
        gl_PrimitiveIndicesNV[Iterator] = Index0;
        gl_PrimitiveIndicesNV[Iterator + 1] = Index1;
        gl_PrimitiveIndicesNV[Iterator + 2] = Index2;
#endif // g_UseExternalMeshShader
    }
}
