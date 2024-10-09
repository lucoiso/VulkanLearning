// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

#ifndef _TYPES_H_
#define _TYPES_H_

#define g_NumTasks 32
#define g_NumVertices 64
#define g_NumPrimitives 124
#define g_NumIndices g_NumPrimitives * 3
#define g_MeshletPerTask 32

#define g_UseExternalMeshShader 0

const uint g_MaxVertexIterations = ((g_NumVertices + g_NumTasks - 1) / g_NumTasks);
const uint g_MaxIndexIterations = ((g_NumPrimitives + g_NumTasks - 1) / g_NumTasks);
const uint g_MaxMeshletIterations = ((g_MeshletPerTask + g_NumTasks - 1) / g_NumTasks);

struct Vertex // std430
{
    vec2 UV;
    vec3 Position;
    vec3 Normal;
    vec4 Color;
    vec4 Joint;
    vec4 Weight;
    vec4 Tangent;
};

struct Meshlet // std430
{
    uint NumVertices;
    uint NumIndices;
    Vertex Vertices[g_NumVertices];
    uint Indices[g_NumIndices];
};

struct ModelUBO // std140
{
    uint NumMeshlets;
    mat4 ProjectionView;
    mat4 ModelView;
};

struct LightingUBO // std140
{
    vec3 LightPosition;
    vec3 LightColor;
    vec3 LightDiffuse;
    vec3 LightAmbient;
    vec3 LightSpecular;
};

struct MaterialUBO // std140
{
    uint8_t AlphaMode;
    bool DoubleSided;
    float MetallicFactor;
    float RoughnessFactor;
    float AlphaCutoff;
    float NormalScale;
    float OcclusionStrength;
    vec3 EmissiveFactor;
    vec4 BaseColorFactor;
};

struct FragmentData
{
    vec4 FragColor;
    vec3 FragView;
    vec3 FragNormal;
    vec2 FragUV;
};

#define g_MaxColors 10
vec3 MeshletColors[g_MaxColors] = {
    vec3(1,0,0),
    vec3(0,1,0),
    vec3(0,0,1),
    vec3(1,1,0),
    vec3(1,0,1),
    vec3(0,1,1),
    vec3(1,0.5,0),
    vec3(0.5,1,0),
    vec3(0,0.5,1),
    vec3(1,1,1)
};

vec3 GetPosition(Meshlet Part, uint VertexIndex)
{
    return Part.Vertices[VertexIndex].Position;
}

vec3 GetNormal(Meshlet Part, uint VertexIndex)
{
    return Part.Vertices[VertexIndex].Normal;
}

vec2 GetUV(Meshlet Part, uint VertexIndex)
{
    return Part.Vertices[VertexIndex].UV;
}

#endif // _TYPES_H_