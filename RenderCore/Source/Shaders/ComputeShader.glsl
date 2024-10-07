#version 460

#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#extension GL_GOOGLE_include_directive : require

#include "Types.h"

layout(std140, set = 0, binding = 0) uniform SceneUniformData
{
    vec3 LightPosition;
    vec3 LightColor;
    vec3 LightDiffuse;
    vec3 LightAmbient;
    vec3 LightSpecular;
} SceneData;

layout(std140, set = 0, binding = 1) uniform MaterialUniformData
{
    vec4 BaseColorFactor;
    vec3 EmissiveFactor;
    float MetallicFactor;
    float RoughnessFactor;
    float AlphaCutoff;
    float NormalScale;
    float OcclusionStrength;
    uint8_t AlphaMode;
    bool DoubleSided;
} MaterialData;

void main() {
    // TODO : implement
}
