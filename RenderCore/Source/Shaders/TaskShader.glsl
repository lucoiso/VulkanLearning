#version 460

#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#extension GL_GOOGLE_include_directive : require

#include "Types.h"

#extension GL_EXT_mesh_shader : require

layout(local_size_x = g_NumTasks) in;

void main()
{
    EmitMeshTasksEXT(g_MaxMeshletIterations, 1, 1);
}
