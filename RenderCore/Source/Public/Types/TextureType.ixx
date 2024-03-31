// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <cstdint>

export module RenderCore.Types.TextureType;

namespace RenderCore
{
    export enum class TextureType : std::uint8_t {
        BaseColor,
        Normal,
        Occlusion,
        Emissive,
        MetallicRoughness,
    };
} // namespace RenderCore
