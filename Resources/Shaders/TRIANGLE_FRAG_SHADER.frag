#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec3 inTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main()
{
    outColor = texture(texSampler, vec2(inTexCoord[0], inTexCoord[1]));
    outColor.rgb *= vec3(inColor[0], inColor[1], inColor[2]);
}