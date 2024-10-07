#version 460
#extension GL_EXT_mesh_shader : require
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : require

layout(local_size_x = 32) in;

layout(std140, set = 1, binding = 0) uniform ModelUniformData
{
    mat4 ProjectionView;
    mat4 ModelView;
    uint NumMeshlets;
} ModelData;

void main()
{
    if (ModelData.NumMeshlets > 0)
    {
        EmitMeshTasksEXT(ModelData.NumMeshlets, 1, 1);
    }
}
