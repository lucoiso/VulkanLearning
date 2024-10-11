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

layout(set = 6, binding = 0) uniform sampler2D BaseColorMap;
layout(set = 6, binding = 1) uniform sampler2D NormalMap;
layout(set = 6, binding = 2) uniform sampler2D OcclusionMap;
layout(set = 6, binding = 3) uniform sampler2D EmissiveMap;
layout(set = 6, binding = 4) uniform sampler2D MetallicRoughnessMap;

layout(location = 0) in FragSharedData
{
    FragmentData Data;
} FragData;

layout(location = 0) out vec4 OutFragColor;

void main()
{
    OutFragColor = FragData.Data.FragColor;
}
