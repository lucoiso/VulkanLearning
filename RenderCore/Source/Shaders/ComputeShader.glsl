#version 450

#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_scalar_block_layout : require

#include "Types.h"

// Debugging
// #extension GL_EXT_debug_printf : enable

layout(std430, set = 0, binding = 0) uniform SceneUniformData
{
    LightingUBO SceneData;
};

layout(std430, set = 2, binding = 0) uniform MaterialUniformData
{
    MaterialUBO MaterialData;
};

void main()
{
    // TODO : implement
}
