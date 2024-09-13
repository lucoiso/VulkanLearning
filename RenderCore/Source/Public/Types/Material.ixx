// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Types.Material;

namespace RenderCore
{
    export enum class AlphaMode : std::uint8_t { ALPHA_OPAQUE, ALPHA_MASK, ALPHA_BLEND };

    export struct RENDERCOREMODULE_API MaterialData
    {
        glm::vec4 BaseColorFactor {};
        glm::vec3 EmissiveFactor {};
        float     MetallicFactor {};
        float     RoughnessFactor {};
        float     AlphaCutoff {};
        float     NormalScale {};
        float     OcclusionStrength {};
        AlphaMode AlphaMode {};
        bool      DoubleSided {};

        inline bool operator==(MaterialData const &Rhs) const
        {
            return BaseColorFactor == Rhs.BaseColorFactor && EmissiveFactor == Rhs.EmissiveFactor && MetallicFactor == Rhs.MetallicFactor &&
                   RoughnessFactor == Rhs.RoughnessFactor && AlphaCutoff == Rhs.AlphaCutoff && NormalScale == Rhs.NormalScale && OcclusionStrength ==
                   Rhs.OcclusionStrength && AlphaMode == Rhs.AlphaMode && DoubleSided == Rhs.DoubleSided;
        }

        inline bool operator!=(MaterialData const &Rhs) const
        {
            return !(*this == Rhs);
        }
    };
} // namespace RenderCore
