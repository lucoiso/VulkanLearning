// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

export module RenderCore.Window.Flags;

export import <cstdint>;
export import RenderCore.Utils.EnumHelpers;

namespace RenderCore
{
    export enum class InitializationFlags : std::uint8_t
    {
        NONE        = 0,
        MAXIMIZED   = 1 << 0,
        HEADLESS    = 1 << 1,
    };
}// namespace RenderCore