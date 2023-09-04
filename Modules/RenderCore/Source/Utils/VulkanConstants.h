// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#ifndef VULKANCONSTANTS_H
#define VULKANCONSTANTS_H

#pragma once

#include <array>
#include <vulkan/vulkan_core.h>

namespace RenderCore
{
    constexpr std::array<const char*, 1> g_ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    constexpr std::array<const char*, 1> g_RequiredExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    constexpr std::array<VkDynamicState, 2> g_DynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    constexpr std::uint32_t g_MaxFramesInFlight = 2u;
}

#endif