#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inColor;
layout(location = 4) in vec4 inTangent;

layout(std140, set = 0, binding = 0) uniform UBOCamera {
    mat4 projection_view; // Combined projection and view matrix
} uboCamera;

layout(std140, set = 0, binding = 2) uniform UBOModel {
    mat4 model;
} uboModel;

layout(location = 1) out FragmentData {
    vec2 model_uv;
    vec3 model_view;
    vec3 model_normal;
    vec4 model_color;
    vec4 model_tangent;
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
}
