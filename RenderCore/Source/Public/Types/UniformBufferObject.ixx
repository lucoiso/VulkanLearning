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
        glm::mat4 Projection{};
        glm::mat4 View{};
        glm::vec3 LightPosition{};
        glm::vec3 LightColor{};
    };

    export struct ModelUniformData
    {
        glm::mat4 ModelMatrix{};
    };

    export struct MaterialUniformData
    {
        glm::vec4 BaseColorFactor{};
        glm::vec3 EmissiveFactor{};
        double    MetallicFactor{};
        double    RoughnessFactor{};
        double    AlphaCutoff{};
        double    NormalScale{};
        double    OcclusionStrength{};
        int       AlphaMode{};
        int       DoubleSided{};
    };
} // namespace RenderCore
