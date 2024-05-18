#version 450

layout(set = 2, binding = 0) uniform sampler2D samplerColorMap;
layout(set = 2, binding = 1) uniform sampler2D samplerNormalMap;
layout(set = 2, binding = 2) uniform sampler2D samplerOcclusionMap;
layout(set = 2, binding = 3) uniform sampler2D samplerEmissiveMap;
layout(set = 2, binding = 4) uniform sampler2D samplerMetallicRoughnessMap;

layout(location = 0) out vec4 outFragColor;

layout(location = 1) in FragmentData {
    vec2  model_uv;
    vec3  model_view;
    vec3  model_normal;
    vec4  model_color;
    vec4  model_tangent;
    vec4  material_baseColorFactor;
    vec3  material_emissiveFactor;
    float material_metallicFactor;
    float material_roughnessFactor;
    float material_alphaCutoff;
    float material_normalScale;
    float material_occlusionStrength;
    int   material_alphaMode;
    int   material_doubleSided;
    vec3  light_position;
    vec3  light_color;
    float light_ambient;
} fragData;

void main() {
    vec4 baseColor = texture(samplerColorMap, fragData.model_uv) * fragData.model_color;
    vec3 normal = normalize(texture(samplerNormalMap, fragData.model_uv).rgb * 2.0 - 1.0);
    normal = normalize(fragData.model_normal);

    vec3 lightDir = normalize(fragData.light_position - fragData.model_view.xyz);
    vec3 lightColor = fragData.light_color;

    float NdotL = max(dot(normal, lightDir), 0.0);

    vec3 diffuse = baseColor.rgb * lightColor * NdotL;

    float occlusion = texture(samplerOcclusionMap, fragData.model_uv).r;
    vec3 ambient = baseColor.rgb * (0.1 + fragData.light_ambient * occlusion);

    vec3 emissive = texture(samplerEmissiveMap, fragData.model_uv).rgb * fragData.material_emissiveFactor;

    outFragColor = vec4(ambient + diffuse + emissive, baseColor.a);
}
