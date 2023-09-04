// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "RenderCoreModule.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace RenderCore
{
    struct RENDERCOREMODULE_API UniformBufferObject
    {
        glm::mat4 Model;
        glm::mat4 View;
        glm::mat4 Projection;
    };
}