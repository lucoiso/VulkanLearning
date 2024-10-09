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

layout(std430, set =  3, binding = 0) buffer MeshletsData        { Meshlet Meshlets       []; };
layout(std430, set =  4, binding = 0) buffer IndicesData         { uint    Indices        []; };
layout(std430, set =  5, binding = 0) buffer VertexUVsData       { vec2    VertexUVs      []; };
layout(std430, set =  6, binding = 0) buffer VertexPositionsData { vec3    VertexPositions[]; };
layout(std430, set =  7, binding = 0) buffer VertexNormalsData   { vec3    VertexNormals  []; };
layout(std430, set =  8, binding = 0) buffer VertexColorsData    { vec4    VertexColors   []; };
layout(std430, set =  9, binding = 0) buffer VertexJointsData    { vec4    VertexJoints   []; };
layout(std430, set = 10, binding = 0) buffer VertexWeightsData   { vec4    VertexWeights  []; };
layout(std430, set = 11, binding = 0) buffer VertexTangentsData  { vec4    VertexTangents []; };

layout(location = 0) out FragSharedData
{
    FragmentData Data;
} FragData[g_NumVertices];

void main()
{
    uint MeshletIndex = gl_WorkGroupID.x;

    Meshlet CurrentMeshlet = Meshlets[MeshletIndex];
    mat4 ModelViewProjection = ModelData.Data.ProjectionView * ModelData.Data.ModelView;

    vec3 MeshletColor = MeshletColors[MeshletIndex % g_MaxColors];

#if g_UseExternalMeshShader
    SetMeshOutputsEXT(CurrentMeshlet.VertexCount, CurrentMeshlet.IndexCount);
#endif // g_UseExternalMeshShader

    [[unroll]]
    for (uint VertexIt = 0; VertexIt < CurrentMeshlet.VertexCount; ++VertexIt)
    {
        uint VertexIndex = CurrentMeshlet.VertexOffset + VertexIt;

        vec4 WorldPosition = ModelData.Data.ModelView * vec4(VertexPositions[VertexIndex], 1.0);
        FragData[VertexIt].Data.FragView = WorldPosition.xyz;
        FragData[VertexIt].Data.FragNormal = VertexNormals[VertexIndex];
        FragData[VertexIt].Data.FragUV = VertexUVs[VertexIndex];
        FragData[VertexIt].Data.FragColor = vec4(MeshletColor, 1.0);

#if g_UseExternalMeshShader
        gl_MeshVerticesEXT[VertexIt].gl_Position = ModelViewProjection * vec4(VertexPositions[VertexIndex], 1.0);
#else
        gl_MeshVerticesNV[VertexIt].gl_Position = ModelViewProjection * vec4(VertexPositions[VertexIndex], 1.0);
#endif // g_UseExternalMeshShader
    }

    [[unroll]]
    for (uint IndexIt = 0; IndexIt < CurrentMeshlet.IndexCount; IndexIt += 3)
    {
        uint Index0 = Indices[CurrentMeshlet.IndexOffset + IndexIt];
        uint Index1 = Indices[CurrentMeshlet.IndexOffset + IndexIt + 1];
        uint Index2 = Indices[CurrentMeshlet.IndexOffset + IndexIt + 2];
        
#if g_UseExternalMeshShader
        gl_PrimitiveTriangleIndicesEXT[IndexIt / 3] = uvec3(Index0, Index1, Index2);
#else
        gl_PrimitiveIndicesNV[gl_PrimitiveCountNV * 3 + 0] = Index0;
        gl_PrimitiveIndicesNV[gl_PrimitiveCountNV * 3 + 1] = Index1;
        gl_PrimitiveIndicesNV[gl_PrimitiveCountNV * 3 + 2] = Index2;
        
        ++gl_PrimitiveCountNV;
#endif // g_UseExternalMeshShader
    }
}
