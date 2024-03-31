// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <glm/ext.hpp>

export module RenderCore.Types.UniformBufferObject;

namespace RenderCore
{
    export struct SceneUniformData
    {
        glm::mat4 Projection {};
        glm::mat4 View {};
        glm::vec4 LightPosition {};
        glm::vec4 LightColor {};
    };
} // namespace RenderCore
