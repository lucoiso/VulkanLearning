// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <cstdint>

export module RenderCore.Window.Flags;

import RenderCore.Utils.EnumHelpers;

namespace RenderCore
{
    export enum class InitializationFlags : std::uint8_t
    {
        NONE      = 0,
        MAXIMIZED = 1 << 0,
        HEADLESS  = 1 << 1,
    };
} // namespace RenderCore
