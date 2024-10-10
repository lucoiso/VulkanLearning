#version 460

#extension GL_EXT_control_flow_attributes : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require

#include "Types.h"

#extension GL_EXT_mesh_shader : require

layout(local_size_x = g_NumTasks) in;

layout(max_vertices = g_NumVertices, max_primitives = g_NumPrimitives) out;
layout(triangles) out;

layout(std140, set = 1, binding = 0) uniform ModelUniformData
{
    ModelUBO ModelData;
};

layout(std430, set = 3, binding = 0) readonly buffer MeshletBuffer
{
    Meshlet Meshlets[];
};

layout(std430, set = 4, binding = 0) readonly buffer IndicesBuffer
{
    uint Indices[];
};

layout(std430, set = 5, binding = 0) readonly buffer VerticesBuffer
{
    Vertex Vertices[];
};

layout(location = 0) out FragSharedData
{
    FragmentData Data;
} FragData[g_NumVertices];

void main()
{
    uint MeshletIndex = gl_WorkGroupID.x;
    uint GroupSize = gl_WorkGroupSize.x;
    uint LocalInvocation = gl_LocalInvocationIndex;

    Meshlet CurrentMeshlet = Meshlets[MeshletIndex];

    uint NumVertices = CurrentMeshlet.VertexCount;
    uint NumIndices = CurrentMeshlet.IndexCount;
    mat4 ModelViewProjection = ModelData.ProjectionView * ModelData.ModelView;
    vec3 MeshletColor = MeshletColors[MeshletIndex % g_MaxColors];

    SetMeshOutputsEXT(NumVertices, NumIndices);

    [[unroll]]
    for (uint VertexIt = LocalInvocation; VertexIt < NumVertices; VertexIt += GroupSize)
    {
        uint VertexIndex = CurrentMeshlet.VertexOffset + VertexIt;
        
        vec4 WorldPosition = ModelData.ModelView * vec4(Vertices[VertexIndex].Position, 1.0);
        FragData[VertexIt].Data.FragView = WorldPosition.xyz;
        FragData[VertexIt].Data.FragNormal = Vertices[VertexIndex].Normal;
        FragData[VertexIt].Data.FragUV = Vertices[VertexIndex].UV;
        FragData[VertexIt].Data.FragColor = vec4(MeshletColor, 1.0);

        gl_MeshVerticesEXT[VertexIt].gl_Position = ModelViewProjection * vec4(Vertices[VertexIndex].Position, 1.0);
    }

    [[unroll]]
    for (uint IndexIt = LocalInvocation; IndexIt < NumIndices; IndexIt += GroupSize)
    {
        uint Offset = CurrentMeshlet.IndexOffset + IndexIt * 3;

        uint Index0 = Indices[Offset + 0];
        uint Index1 = Indices[Offset + 1];
        uint Index2 = Indices[Offset + 2];
        
        gl_PrimitiveTriangleIndicesEXT[IndexIt] = uvec3(Index0, Index1, Index2);
    }
}