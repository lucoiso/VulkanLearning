#version 450

#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_scalar_block_layout : require

#include "Types.h"

#extension GL_EXT_mesh_shader : require

// Debugging
// #extension GL_EXT_debug_printf : enable

layout(local_size_x = g_NumTasks) in;

void main()
{
    EmitMeshTasksEXT(g_MaxMeshletIterations, 1, 1);
}
