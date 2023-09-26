// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "Utils/EnumHelpers.h"

namespace RenderCore
{    
    enum class ApplicationEventFlags : std::uint8_t
    {
        NONE                    = 0,
        DRAW_FRAME              = 1 << 0,
        LOAD_SCENE              = 1 << 1,
        UNLOAD_SCENE            = 1 << 2
    };
    DECLARE_BITWISE_OPERATORS(ApplicationEventFlags);
}