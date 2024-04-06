#version 450

layout(std140, set = 0, binding = 1) uniform UBOLight {
    vec3 position;
    vec3 color;
} uboLight;

layout(std140, set = 0, binding = 3) uniform UBOMaterial {
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

layout(set = 0, binding = 4) uniform sampler2D samplerColorMap;
layout(set = 0, binding = 5) uniform sampler2D samplerNormalMap;
layout(set = 0, binding = 6) uniform sampler2D samplerOcclusionMap;
layout(set = 0, binding = 7) uniform sampler2D samplerEmissiveMap;
layout(set = 0, binding = 8) uniform sampler2D samplerMetallicRoughnessMap;

layout(location = 0) out vec4 outFragColor;

layout(location = 1) in FragmentData {
    vec2 model_uv;
    vec3 model_view;
    vec3 cam_view;
    vec3 model_normal;
    vec4 model_color;
    vec4 model_tangent;
} fragData;

void main() {
    vec4 baseColor = texture(samplerColorMap, fragData.model_uv);
    vec3 normal = normalize(texture(samplerNormalMap, fragData.model_uv).rgb * 2.0 - 1.0);
    normal = normalize(fragData.model_normal);

    vec3 viewDir = normalize(fragData.cam_view);
    vec3 lightDir = normalize(uboLight.position - fragData.model_view);
    vec3 lightColor = uboLight.color;

    float NdotL = max(dot(normal, lightDir), 0.0);

    vec3 diffuse = baseColor.rgb * lightColor * NdotL;

    float occlusion = texture(samplerOcclusionMap, fragData.model_uv).r;
    vec3 ambient = baseColor.rgb * (0.1 + 0.9 * occlusion);

    vec3 emissive = texture(samplerEmissiveMap, fragData.model_uv).rgb * uboMaterial.emissiveFactor;

    vec3 result = ambient + diffuse + emissive;
    outFragColor = vec4(result, baseColor.a);
}
