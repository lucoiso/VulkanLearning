// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

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

        Vertex(const glm::vec3 &InPosition, const glm::vec3 &InColor, const glm::vec2 &InTextureCoordinate)
            : Position(InPosition)
            , Color(InColor)
            , TextureCoordinate(InTextureCoordinate) {}
    };
}