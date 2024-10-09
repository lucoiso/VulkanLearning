#version 460

#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#extension GL_GOOGLE_include_directive : require

#include "Types.h"

layout(std140, set = 0, binding = 0) uniform SceneUniformData
{
    LightingUBO Data;
} SceneData;

layout(std140, set = 2, binding = 0) uniform MaterialUniformData
{
    MaterialUBO Data;
} MaterialData;

layout(set = 12, binding = 0) uniform sampler2D BaseColorMap;
layout(set = 12, binding = 1) uniform sampler2D NormalMap;
layout(set = 12, binding = 2) uniform sampler2D OcclusionMap;
layout(set = 12, binding = 3) uniform sampler2D EmissiveMap;
layout(set = 12, binding = 4) uniform sampler2D MetallicRoughnessMap;

layout(location = 0) in FragSharedData
{
    FragmentData Data;
} FragData;

layout(location = 0) out vec4 OutFragColor;

void main()
{
    OutFragColor = FragData.Data.FragColor;
}
