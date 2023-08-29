// Copyright Notice: [...]

#ifndef VULKANCONSTANTS_H
#define VULKANCONSTANTS_H

#pragma once

#include <array>

namespace RenderCore
{
    constexpr std::array<const char*, 1> g_ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    constexpr std::array<const char*, 1> g_RequiredExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
}

#endif