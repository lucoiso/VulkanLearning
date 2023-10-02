// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include "RenderCoreModule.h"

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif
#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

export module RenderCore.Types.VulkanUniformBufferObject;

namespace RenderCore
{
    export struct RENDERCOREMODULE_API UniformBufferObject
    {
        glm::mat4 ModelViewProjection;
    };
}// namespace RenderCore
