#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec3 inColor;
layout(location = 4) in vec4 inTangent;

layout(set = 0, binding = 0) uniform UBOScene {
    mat4 projection;
    mat4 view;
    vec4 lightPos;
    vec4 lightColor;
} uboScene;

layout(set = 0, binding = 1) uniform UBOModel {
    mat4 local;
} uboModel;

layout(location = 1) out VS_OUT {
    vec3 fragNormal;
    vec4 fragColor;
    vec2 fragUV;
    vec3 fragTangent;
    vec3 fragViewDir;
    vec3 fragLightDir;
    vec3 fragLightColor;
} vs_out;

void main() {
    gl_Position = uboScene.projection * uboScene.view * uboModel.local * vec4(inPos, 1.0);

    vec4 worldPos = uboModel.local * vec4(inPos, 1.0);
    vec3 worldNormal = mat3(uboModel.local) * inNormal;
    vec3 worldTangent = normalize(mat3(uboModel.local) * inTangent.xyz);
    vec3 worldBitangent = cross(worldNormal, worldTangent) * inTangent.w;

    vs_out.fragNormal = normalize(worldNormal);
    vs_out.fragColor = vec4(inColor, 1.0);
    vs_out.fragUV = inUV;
    vs_out.fragTangent = worldTangent;

    vec3 worldViewDir = normalize(vec3(uboScene.view * vec4(0.0, 0.0, 0.0, 1.0)) - worldPos.xyz);
    vec3 lightDir = normalize(uboScene.lightPos.xyz - worldPos.xyz);

    vs_out.fragViewDir = worldViewDir;
    vs_out.fragLightDir = lightDir;
    vs_out.fragLightColor = uboScene.lightColor.rgb;
}
