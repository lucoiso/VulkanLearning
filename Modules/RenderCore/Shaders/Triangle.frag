#version 450

// https://github.com/SaschaWillems/Vulkan/blob/master/shaders/glsl/triangle/triangle.frag

layout (location = 0) in vec3 inColor;

layout (location = 0) out vec4 outFragColor;

void main() 
{
  outFragColor = vec4(inColor, 1.0);
}