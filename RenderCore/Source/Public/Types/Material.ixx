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
        glm::uint8   AlphaMode{};
        glm::uint8   DoubleSided{};
        glm::uint8   _Padding1[2];
        glm::float32 MetallicFactor {};
        glm::float32 RoughnessFactor {};
        glm::float32 AlphaCutoff {};
        glm::float32 NormalScale {};
        glm::float32 OcclusionStrength {};
        glm::float32 _Padding2[2];
        glm::vec3    EmissiveFactor{};
        glm::float32 _Padding3[1];
        glm::vec4    BaseColorFactor{};

        inline bool operator==(MaterialData const &Rhs) const
        {
            return BaseColorFactor == Rhs.BaseColorFactor &&
                   EmissiveFactor == Rhs.EmissiveFactor &&
                   MetallicFactor == Rhs.MetallicFactor &&
                   RoughnessFactor == Rhs.RoughnessFactor &&
                   AlphaCutoff == Rhs.AlphaCutoff &&
                   NormalScale == Rhs.NormalScale &&
                   OcclusionStrength == Rhs.OcclusionStrength &&
                   AlphaMode == Rhs.AlphaMode &&
                   DoubleSided == Rhs.DoubleSided;
        }
    };
} // namespace RenderCore
