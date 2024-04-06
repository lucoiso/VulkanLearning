#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inColor;
layout(location = 4) in vec4 inTangent;

layout(set = 0, binding = 0) uniform UBOScene {
    mat4 projection;
    mat4 view;
    vec4 lightPos;
    vec4 lightColor;
} uboScene;

layout(set = 0, binding = 1) uniform UBOModel {
    mat4 model;
} uboModel;

layout(location = 1) out FragmentData {
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
    vec4 modelViewPos = uboScene.view * uboModel.model * vec4(inPos, 1.0);

    fragData.model_view = modelViewPos.xyz;
    fragData.model_normal = inNormal;
    fragData.model_color = inColor;
    fragData.model_uv = inUV;
    fragData.model_tangent = inTangent;
    fragData.cam_view = (uboScene.view * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    fragData.light_view = uboScene.lightPos.xyz;
    fragData.light_color = uboScene.lightColor.xyz;

    gl_Position = uboScene.projection * modelViewPos;
}
