// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <glm/ext.hpp>

export module RenderCore.Types.UniformBufferObject;

namespace RenderCore
{
    export struct SceneUniformData
    {
        alignas(16) glm::mat4 ProjectionView {};
        alignas(16) glm::vec3 LightPosition {};
        alignas(16) glm::vec3 LightColor {};
        alignas(8) double     AmbientLight {};
    };

    export struct ModelUniformData
    {
        alignas(16) glm::mat4   Model {};
        alignas(16) glm::vec4   BaseColorFactor {};
        alignas(16) glm::vec3   EmissiveFactor {};
        alignas(8) double       MetallicFactor {};
        alignas(8) double       RoughnessFactor {};
        alignas(8) double       AlphaCutoff {};
        alignas(8) double       NormalScale {};
        alignas(8) double       OcclusionStrength {};
        alignas(4) std::int32_t AlphaMode {};
        alignas(4) std::int32_t DoubleSided {};
    };
} // namespace RenderCore
