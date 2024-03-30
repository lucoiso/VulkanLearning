#version 450

layout(set = 0, binding = 2) uniform sampler2D samplerColorMap;
layout(set = 0, binding = 3) uniform sampler2D samplerNormalMap;
layout(set = 0, binding = 4) uniform sampler2D samplerOcclusionMap;
layout(set = 0, binding = 5) uniform sampler2D samplerEmissiveMap;
layout(set = 0, binding = 6) uniform sampler2D samplerMetallicRoughnessMap;

layout(location = 1) in VS_OUT {
    vec3 fragNormal;
    vec4 fragColor;
    vec2 fragUV;
    vec3 fragTangent;
    vec3 fragViewDir;
    vec3 fragLightDir;
    vec3 fragLightColor;
} fs_in;

layout(location = 0) out vec4 outFragColor;

void main() {
    vec4 baseColor = texture(samplerColorMap, fs_in.fragUV);
    vec3 normal = normalize(texture(samplerNormalMap, fs_in.fragUV).rgb * 2.0 - 1.0);
    vec3 tangent = normalize(fs_in.fragTangent);
    vec3 bitangent = normalize(cross(fs_in.fragNormal, fs_in.fragTangent));
    mat3 TBN = mat3(tangent, bitangent, fs_in.fragNormal);
    normal = normalize(TBN * normal);

    vec3 viewDir = normalize(fs_in.fragViewDir);
    vec3 lightDir = normalize(fs_in.fragLightDir);
    vec3 lightColor = fs_in.fragLightColor;

    float diffuseStrength = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = baseColor.rgb * lightColor * diffuseStrength;

    float specularStrength = 0.5;
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = vec3(1.0) * spec * specularStrength;

    float occlusion = texture(samplerOcclusionMap, fs_in.fragUV).r;
    vec3 ambient = baseColor.rgb * (0.1 + 0.9 * occlusion);

    vec3 emissive = texture(samplerEmissiveMap, fs_in.fragUV).rgb;

    vec2 metallicRoughness = texture(samplerMetallicRoughnessMap, fs_in.fragUV).rg;
    float metallic = metallicRoughness.r;
    float roughness = metallicRoughness.g;

    vec3 result = ambient + diffuse + specular + emissive;
    outFragColor = vec4(result, baseColor.a);
}