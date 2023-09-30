// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

namespace RenderCore
{
    enum class ApplicationEventFlags : std::uint8_t
    {
        NONE,
        DRAW_FRAME,
        LOAD_SCENE,
        UNLOAD_SCENE,
        MAX
    };
}
