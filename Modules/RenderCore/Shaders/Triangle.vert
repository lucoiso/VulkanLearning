#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 Model;
    mat4 View;
    mat4 Projection;
} UBO;

layout(location = 0) in vec2 InputPosition;
layout(location = 1) in vec3 InputColor;

layout(location = 0) out vec3 FragmentColor;

void main() {
    // TODO: Bind Object - gl_Position = UBO.Model * UBO.View * UBO.Projection * vec4(InputPosition, 0.0, 1.0);
    gl_Position = vec4(InputPosition, 0.0, 1.0);
    FragmentColor = InputColor;
}