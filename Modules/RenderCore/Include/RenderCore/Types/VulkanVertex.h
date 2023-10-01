// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

#pragma once

#include "RenderCoreModule.h"
#include <glm/glm.hpp>

namespace RenderCore
{
    struct RENDERCOREMODULE_API Vertex
    {
        glm::vec3 Position;
        glm::vec3 Color;
        glm::vec2 TextureCoordinate;

        Vertex() = default;

        Vertex(glm::vec3 const& InPosition, glm::vec3 const& InColor, glm::vec2 const& InTextureCoordinate)
            : Position(InPosition),
              Color(InColor),
              TextureCoordinate(InTextureCoordinate)
        {
        }
    };
}
