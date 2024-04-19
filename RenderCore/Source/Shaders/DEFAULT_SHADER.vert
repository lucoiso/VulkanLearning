#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inColor;
layout(location = 4) in vec4 inTangent;

layout(std140, set = 0, binding = 0) uniform UBOCamera {
    mat4 projection_view;
    vec3 light_position;
    vec3 light_color;
} uboCamera;

layout(std140, set = 1, binding = 0) uniform UBOModel {
    mat4  model;
    vec4  material_baseColorFactor;
    vec3  material_emissiveFactor;
    float material_metallicFactor;
    float material_roughnessFactor;
    float material_alphaCutoff;
    float material_normalScale;
    float material_occlusionStrength;
    int   material_alphaMode;
    int   material_doubleSided;
} uboModel;

layout(location = 1) out FragmentData {
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
} fragData;

void main() {
    vec4 worldPos = uboModel.model * vec4(inPos, 1.0);
    vec4 viewPos = uboCamera.projection_view * worldPos;
    gl_Position = viewPos;

    fragData.model_uv = inUV;
    fragData.model_view = viewPos.xyz;
    fragData.model_normal = normalize(mat3(uboModel.model) * inNormal);
    fragData.model_color = inColor;
    fragData.model_tangent = inTangent;

    fragData.material_baseColorFactor = uboModel.material_baseColorFactor;
    fragData.material_emissiveFactor = uboModel.material_emissiveFactor;
    fragData.material_metallicFactor = uboModel.material_metallicFactor;
    fragData.material_roughnessFactor = uboModel.material_roughnessFactor;
    fragData.material_alphaCutoff = uboModel.material_alphaCutoff;
    fragData.material_normalScale = uboModel.material_normalScale;
    fragData.material_occlusionStrength = uboModel.material_occlusionStrength;
    fragData.material_alphaMode = uboModel.material_alphaMode;
    fragData.material_doubleSided = uboModel.material_doubleSided;

    fragData.light_position = uboCamera.light_position;
    fragData.light_color = uboCamera.light_color;
}
