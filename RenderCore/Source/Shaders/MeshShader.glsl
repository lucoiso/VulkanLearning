#version 460
#extension GL_EXT_mesh_shader : require
#extension GL_EXT_control_flow_attributes : require

layout(local_size_x = 32) in;

layout(max_vertices = 64, max_primitives = 64) out;
layout(triangles) out;

// Número máximo de iterações baseado no tamanho do meshlet
const uint g_MaxIterations = ((64 + 32 - 1) / 32);

// Estruturas de dados
struct Vertex
{
    vec3 Position;
    vec3 Normal;
    vec2 UV;
    vec4 Joint;
    vec4 Weight;
    vec4 Tangent;
};

struct Meshlet
{
    Vertex Vertices[64];
    uint Indices[378];
    uint NumVertices;
    uint NumIndices;
};

// Dados de uniformes do modelo
layout(std140, set = 1, binding = 0) uniform ModelUniformData
{
    mat4 ProjectionView;
    mat4 ModelView;
    uint NumMeshlets;
} ModelData;

layout(std140, set = 3, binding = 0) buffer MeshletBuffer
{
    Meshlet Buffer[];
} MeshletData;

layout(location = 0) out FragmentData
{
    vec4 FragPosition;
    vec3 FragNormal;
    vec2 FragUV;
    vec4 FragColor;
} FragData[64];

// Cores para os meshlets
#define MAX_COLORS 10
vec3 MeshletColors[MAX_COLORS] = {
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

// Funções para acessar dados do meshlet
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

void main()
{
    uint MeshletIndex = gl_WorkGroupID.x;
    if (MeshletIndex >= ModelData.NumMeshlets) return;

    Meshlet CurrentMeshlet = MeshletData.Buffer[MeshletIndex];

    uint NumVertices = CurrentMeshlet.NumVertices;
    uint NumIndices = CurrentMeshlet.NumIndices;

    SetMeshOutputsEXT(NumVertices, NumIndices);

    for (uint i = 0; i < NumVertices; i++)
    {
        vec3 Position = GetPosition(CurrentMeshlet, i);
        vec3 Normal = GetNormal(CurrentMeshlet, i);
        vec2 UV = GetUV(CurrentMeshlet, i);

        vec4 ClipPosition = ModelData.ProjectionView * vec4(Position, 1.0);

        FragData[i].FragPosition = ClipPosition;
        FragData[i].FragNormal = Normal;
        FragData[i].FragUV = UV;

        FragData[i].FragColor = vec4(MeshletColors[MeshletIndex % MAX_COLORS], 1.0);

        gl_MeshVerticesEXT[i].gl_Position = ClipPosition;
    }

    for (uint i = 0; i < NumIndices / 3; i++)
    {
        uint index0 = CurrentMeshlet.Indices[i * 3];
        uint index1 = CurrentMeshlet.Indices[i * 3 + 1];
        uint index2 = CurrentMeshlet.Indices[i * 3 + 2];

        gl_PrimitiveTriangleIndicesEXT[i] = uvec3(index0, index1, index2);
    }
}
