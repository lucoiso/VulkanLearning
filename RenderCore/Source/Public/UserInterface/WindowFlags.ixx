// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <cstdint>

export module RenderCore.UserInterface.Window.Flags;

import RenderCore.Utils.EnumHelpers;

namespace RenderCore
{
    export enum class InitializationFlags : std::uint8_t
    {
        NONE         = 0,
        MAXIMIZED    = 1 << 0,
        HEADLESS     = 1 << 1,
        ENABLE_IMGUI = 1 << 2,
        ENABLE_DOCKING = 1 << 3,
    };
} // namespace RenderCore
