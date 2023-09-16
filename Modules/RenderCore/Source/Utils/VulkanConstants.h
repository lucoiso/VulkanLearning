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
#ifdef _DEBUG
    constexpr std::array<const char*, 1> g_DebugInstanceLayers = {
        "VK_LAYER_KHRONOS_validation"     
    };

    constexpr std::array<const char*, 1> g_DebugDeviceLayers = {
        "VK_LAYER_KHRONOS_validation"     
    };

    constexpr std::array<const char*, 3> g_DebugInstanceExtensions = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        "VK_EXT_debug_report",
        "VK_EXT_validation_features"  
    };

    constexpr std::array<const char*, 2> g_DebugDeviceExtensions = {
        "VK_EXT_validation_cache",
        "VK_EXT_tooling_info"   
    };

    constexpr std::array<VkValidationFeatureEnableEXT, 3> g_EnabledInstanceValidationFeatures = {
        // VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
        // VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
        VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
        VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
    };
#endif

    constexpr std::array<const char*, 0> g_RequiredInstanceLayers = {
    };

    constexpr std::array<const char*, 0> g_RequiredDeviceLayers = {
    };

    constexpr std::array<const char*, 0> g_RequiredInstanceExtensions = {
    };

    constexpr std::array<const char*, 1> g_RequiredDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    constexpr std::array<VkDynamicState, 2> g_DynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    constexpr std::uint8_t g_MaxFramesInFlight = 2u;

    constexpr std::uint32_t g_BufferMemoryAllocationSize = 65536u;
}

#endif