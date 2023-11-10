#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 InFragColor;
layout(location = 1) in vec2 InFragTexCoord;

layout(location = 0) out vec4 OutFragColor;

layout(binding = 1) uniform sampler2D BaseColorTexture;
layout(binding = 2) uniform sampler2D NormalTexture;
layout(binding = 3) uniform sampler2D OcclusionTexture;
layout(binding = 4) uniform sampler2D EmissiveTexture;

void main()
{
    vec4 BaseColor = texture(BaseColorTexture, InFragTexCoord);
    vec3 Normal = texture(NormalTexture, InFragTexCoord).xyz;
    float Occlusion = texture(OcclusionTexture, InFragTexCoord).r;
    vec3 Emissive = texture(EmissiveTexture, InFragTexCoord).xyz;
    vec4 EmissiveColor = vec4(Emissive * InFragColor.rgb, 1.0);

    OutFragColor = BaseColor * (1.0 - Occlusion) + EmissiveColor;
    OutFragColor.rgba *= InFragColor;
}
