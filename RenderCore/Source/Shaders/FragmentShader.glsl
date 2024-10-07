#version 460
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require

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

layout(set = 4, binding = 0) uniform sampler2D BaseColorMap;
layout(set = 4, binding = 1) uniform sampler2D NormalMap;
layout(set = 4, binding = 2) uniform sampler2D OcclusionMap;
layout(set = 4, binding = 3) uniform sampler2D EmissiveMap;
layout(set = 4, binding = 4) uniform sampler2D MetallicRoughnessMap;

layout(location = 0) in FragmentData
{
    vec4 FragPosition;
    vec3 FragNormal;
    vec2 FragUV;
    vec4 FragColor;
} FragData;

layout(location = 0) out vec4 OutFragColor;

void main()
{
    OutFragColor = FragData.FragColor;
}
