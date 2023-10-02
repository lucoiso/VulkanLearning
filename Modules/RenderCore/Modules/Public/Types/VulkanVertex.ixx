// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include "RenderCoreModule.h"
#include <glm/glm.hpp>

export module RenderCore.Types.VulkanVertex;

namespace RenderCore
{
    export struct RENDERCOREMODULE_API Vertex
    {
        glm::vec3 Position;
        glm::vec3 Color;
        glm::vec2 TextureCoordinate;
    };
}// namespace RenderCore
