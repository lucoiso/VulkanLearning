#version 450

#extension GL_EXT_control_flow_attributes : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#extension GL_EXT_scalar_block_layout : require

#include "Types.h"

#extension GL_EXT_mesh_shader : require

// Debugging
// #extension GL_EXT_debug_printf : enable

layout(local_size_x = g_NumTasks) in;

layout(max_vertices = g_NumVertices, max_primitives = g_NumPrimitives) out;
layout(triangles) out;

layout(std430, set = 1, binding = 0) uniform ModelUniformData
{
    ModelUBO ModelData;
};

layout(std430, set = 3, binding = 0) readonly buffer MeshletBuffer
{
    Meshlet Meshlets[];
};

layout(std430, set = 4, binding = 0) readonly buffer IndicesBuffer
{
    uint8_t Indices[];
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
    Meshlet Meshlet = Meshlets[MeshletIndex];

    SetMeshOutputsEXT(Meshlet.VertexCount, Meshlet.TriangleCount);

    [[unroll]]
    for (uint VertexIt = 0; VertexIt < Meshlet.VertexCount; ++VertexIt)
    {
        uint VertexIndex = Meshlet.VertexOffset + VertexIt;
        Vertex Vertex = Vertices[VertexIndex];

        vec4 WorldPosition = ModelData.ModelView * vec4(Vertex.Position, 1.0);
        vec4 ClipPosition = ModelData.ProjectionView * WorldPosition;

        gl_MeshVerticesEXT[VertexIt].gl_Position = ClipPosition;

        FragData[VertexIt].Data.FragUV = Vertex.UV;
        FragData[VertexIt].Data.FragNormal = mat3(transpose(inverse(ModelData.ModelView))) * Vertex.Normal;
        FragData[VertexIt].Data.FragView = -WorldPosition.xyz;
        FragData[VertexIt].Data.FragColor = vec4(MeshletColors[MeshletIndex % g_MaxColors], 1.0);
    }

    [[unroll]]
    for (uint IndexIt = 0; IndexIt < Meshlet.TriangleCount; ++IndexIt)
    {
        uint TriangleIndex = Meshlet.TriangleOffset + IndexIt * 3;

        uint Index0 = Indices[TriangleIndex + 0];
        uint Index1 = Indices[TriangleIndex + 1];
        uint Index2 = Indices[TriangleIndex + 2];

        gl_PrimitiveTriangleIndicesEXT[IndexIt] = uvec3(Index0, Index1, Index2);
    }
}
