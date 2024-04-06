// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <glm/ext.hpp>

export module RenderCore.Types.Material;

import RenderCore.Types.Transform;

namespace RenderCore
{
    export enum class TextureType : std::uint8_t
    {
        BaseColor,
        Normal,
        Occlusion,
        Emissive,
        MetallicRoughness,
    };

    export enum class AlphaMode : std::uint8_t { ALPHA_OPAQUE, ALPHA_MASK, ALPHA_BLEND };

    export struct MaterialData
    {
        glm::vec4 BaseColorFactor{};
        glm::vec3 EmissiveFactor{};
        float     MetallicFactor{};
        float     RoughnessFactor{};
        float     AlphaCutoff{};
        float     NormalScale{};
        float     OcclusionStrength{};
        AlphaMode AlphaMode{};
        bool      DoubleSided{};
    };
} // namespace RenderCore
