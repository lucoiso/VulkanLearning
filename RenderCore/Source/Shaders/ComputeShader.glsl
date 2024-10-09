#version 460

#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require
#extension GL_GOOGLE_include_directive : require

#include "Types.h"

layout(std140, set = 0, binding = 0) uniform SceneUniformData
{
    LightingUBO Data;
} SceneData;

layout(std140, set = 0, binding = 1) uniform MaterialUniformData
{
    MaterialUBO Data;
} MaterialData;

void main() {
    // TODO : implement
}
