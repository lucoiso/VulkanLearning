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
        alignas(1)  AlphaMode    AlphaMode {};
        alignas(1)  bool         DoubleSided{};
        alignas(4)  glm::uint8   Padding[2]{};
        alignas(4)  glm::float32 MetallicFactor {};
        alignas(4)  glm::float32 RoughnessFactor {};
        alignas(4)  glm::float32 AlphaCutoff {};
        alignas(4)  glm::float32 NormalScale {};
        alignas(4)  glm::float32 OcclusionStrength {};
        alignas(16) glm::vec3    EmissiveFactor{};
        alignas(16) glm::vec4    BaseColorFactor{};

        inline bool operator==(MaterialData const &Rhs) const
        {
            return BaseColorFactor == Rhs.BaseColorFactor && EmissiveFactor == Rhs.EmissiveFactor && MetallicFactor == Rhs.MetallicFactor &&
                   RoughnessFactor == Rhs.RoughnessFactor && AlphaCutoff == Rhs.AlphaCutoff && NormalScale == Rhs.NormalScale && OcclusionStrength ==
                   Rhs.OcclusionStrength && AlphaMode == Rhs.AlphaMode && DoubleSided == Rhs.DoubleSided;
        }
    };
} // namespace RenderCore
