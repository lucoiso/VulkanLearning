// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <glm/ext.hpp>

export module RenderCore.Types.UniformBufferObject;

namespace RenderCore
{
    export struct UniformBufferObject
    {
        glm::mat4 Model{};
        glm::mat4 View{};
        glm::mat4 Projection{};
    };
}// namespace RenderCore
