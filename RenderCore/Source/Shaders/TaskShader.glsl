#version 460

#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#extension GL_GOOGLE_include_directive : require

#include "Types.h"

#if g_UseExternalMeshShader
#extension GL_EXT_mesh_shader : require
#else
#extension GL_NV_mesh_shader : require
#endif // g_UseExternalMeshShader

layout(local_size_x = g_NumTasks) in;

layout(std140, set = 1, binding = 0) uniform ModelUniformData
{
    ModelUBO Data;
} ModelData;

void main()
{
    if (ModelData.Data.NumMeshlets > 0)
    {
#if g_UseExternalMeshShader
        EmitMeshTasksEXT(g_MaxMeshletIterations, 1, 1);
#else
        gl_TaskCountNV = g_MaxMeshletIterations;
#endif // g_UseExternalMeshShader
    }
}
