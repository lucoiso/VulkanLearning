#version 460

#extension GL_EXT_mesh_shader : require
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#extension GL_GOOGLE_include_directive : require

#include "Types.h"

layout(local_size_x = g_NumTasks) in;

layout(std140, set = 1, binding = 0) uniform ModelUniformData
{
    ModelUBO Data;
} ModelData;

void main()
{
    if (ModelData.Data.NumMeshlets > 0)
    {
        EmitMeshTasksEXT(ModelData.Data.NumMeshlets, 1, 1);
    }
}
