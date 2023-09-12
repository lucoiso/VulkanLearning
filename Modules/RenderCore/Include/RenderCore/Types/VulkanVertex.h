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
        glm::vec3 Normal;
        glm::vec3 Color;
        glm::vec2 TextureCoordinate;
    };
}