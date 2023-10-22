#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 InFragColor;
layout(location = 1) in vec3 InFragTexCoord;

layout(location = 0) out vec4 OutFragColor;

layout(binding = 1) uniform sampler2D TextureSampler;

void main()
{
    OutFragColor = texture(TextureSampler, vec2(InFragTexCoord[0], InFragTexCoord[1]));
    OutFragColor.rgba *= InFragColor;
}