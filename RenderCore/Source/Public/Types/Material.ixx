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
        alignas(16) glm::vec4 BaseColorFactor {};
        alignas(16) glm::vec3 EmissiveFactor {};
        alignas(4)  float     MetallicFactor {};
        alignas(4)  float     RoughnessFactor {};
        alignas(4)  float     AlphaCutoff {};
        alignas(4)  float     NormalScale {};
        alignas(4)  float     OcclusionStrength {};
        alignas(4)  AlphaMode AlphaMode {};
        alignas(4)  bool      DoubleSided {};

        inline bool operator==(MaterialData const &Rhs) const
        {
            return BaseColorFactor == Rhs.BaseColorFactor && EmissiveFactor == Rhs.EmissiveFactor && MetallicFactor == Rhs.MetallicFactor &&
                   RoughnessFactor == Rhs.RoughnessFactor && AlphaCutoff == Rhs.AlphaCutoff && NormalScale == Rhs.NormalScale && OcclusionStrength ==
                   Rhs.OcclusionStrength && AlphaMode == Rhs.AlphaMode && DoubleSided == Rhs.DoubleSided;
        }
    };
} // namespace RenderCore
