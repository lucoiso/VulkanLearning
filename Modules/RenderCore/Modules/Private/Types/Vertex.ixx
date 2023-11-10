// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <glm/ext.hpp>

export module RenderCore.Types.Vertex;

namespace RenderCore
{
    export struct Vertex
    {
        glm::vec3 Position {};
        glm::vec3 Normal {};
        glm::vec4 Color {};
        glm::vec2 TextureCoordinate {};
    };
}// namespace RenderCore
