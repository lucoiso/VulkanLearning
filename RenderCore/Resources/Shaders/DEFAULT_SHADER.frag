#version 450

layout(set = 0, binding = 2) uniform UBOMaterial {
    vec4 baseColorFactor;
    vec3 emissiveFactor;
    float metallicFactor;
    float roughnessFactor;
    float alphaCutoff;
    float normalScale;
    float occlusionStrength;
    int alphaMode;
    int doubleSided;
} uboMaterial;

layout(set = 0, binding = 3) uniform sampler2D samplerColorMap;
layout(set = 0, binding = 4) uniform sampler2D samplerNormalMap;
layout(set = 0, binding = 5) uniform sampler2D samplerOcclusionMap;
layout(set = 0, binding = 6) uniform sampler2D samplerEmissiveMap;
layout(set = 0, binding = 7) uniform sampler2D samplerMetallicRoughnessMap;

layout(location = 0) out vec4 outFragColor;

layout(location = 1) in FragmentData {
    vec3 model_view;
    vec3 model_normal;
    vec4 model_color;
    vec2 model_uv;
    vec4 model_tangent;
    vec3 cam_view;
    vec3 light_view;
    vec3 light_color;
} fragData;

void main() {
    outFragColor = texture(samplerColorMap, fragData.model_uv) * uboMaterial.baseColorFactor;
}
