#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 Model;
    mat4 View;
    mat4 Projection;
} VertexUBO;

layout(location = 0) in vec3 InVertPosition;
layout(location = 1) in vec3 InVertNormal;
layout(location = 2) in vec4 InVertColor;
layout(location = 3) in vec2 InVertTexCoord;

layout(location = 0) out vec4 OutVertColor;
layout(location = 1) out vec2 OutVertTexCoord;

void main() {
    gl_Position = VertexUBO.Projection * VertexUBO.View * VertexUBO.Model * vec4(InVertPosition, 1.0);
    OutVertColor = InVertColor;
    OutVertTexCoord = InVertTexCoord;
}