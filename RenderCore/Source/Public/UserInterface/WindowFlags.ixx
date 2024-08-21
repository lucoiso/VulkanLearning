// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.UserInterface.Window.Flags;

import RenderCore.Utils.EnumHelpers;

namespace RenderCore
{
    export enum class InitializationFlags : std::uint8_t
    {
        NONE             = 0,
        MAXIMIZED        = 1 << 0,
        HEADLESS         = 1 << 1,
        ENABLE_IMGUI     = 1 << 2,
        ENABLE_VIEWPORTS = 1 << 3,
        ENABLE_DOCKING   = 1 << 4,
        WITHOUT_TITLEBAR = 1 << 5,
        ALWAYS_ON_TOP    = 1 << 6,
    };
} // namespace RenderCore
