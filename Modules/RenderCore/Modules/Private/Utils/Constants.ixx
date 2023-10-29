// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <volk.h>

export module RenderCore.Utils.Constants;

export import <array>;
export import <cstdint>;
import <numeric>;

export namespace RenderCore
{
    constexpr bool g_EnableCustomDebug = false;

#ifdef _DEBUG
#if defined(GPU_API_DUMP) && GPU_API_DUMP
    constexpr std::array g_DebugInstanceLayers = {
            "VK_LAYER_KHRONOS_validation",
            "VK_LAYER_LUNARG_api_dump"};

    constexpr std::array g_DebugDeviceLayers = {
            "VK_LAYER_KHRONOS_validation",
            "VK_LAYER_LUNARG_api_dump"};
#else
    constexpr std::array g_DebugInstanceLayers = {
            "VK_LAYER_KHRONOS_validation"};

    constexpr std::array g_DebugDeviceLayers = {
            "VK_LAYER_KHRONOS_validation"};
#endif

    constexpr std::array g_DebugInstanceExtensions = {
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            "VK_EXT_debug_report",
            "VK_EXT_validation_features"};

    constexpr std::array g_DebugDeviceExtensions = {
            "VK_EXT_validation_cache"};

    constexpr std::array g_EnabledInstanceValidationFeatures = {
            // VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
            // VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
            VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
            VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
            VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT};
#endif

    constexpr std::array<char const*, 0U> g_RequiredInstanceLayers = {};

    constexpr std::array<char const*, 0U> g_RequiredDeviceLayers = {};

    constexpr std::array<char const*, 0U> g_RequiredInstanceExtensions = {};

    constexpr std::array g_RequiredDeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_EXT_ROBUSTNESS_2_EXTENSION_NAME};

    constexpr std::array g_DynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR};

    constexpr std::uint64_t g_BufferMemoryAllocationSize      = 65536U;
    constexpr std::uint64_t g_ImageBufferMemoryAllocationSize = 262144U;

    constexpr VkSampleCountFlagBits g_MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    constexpr std::uint8_t g_FrameRate = 120U;

    constexpr std::uint32_t g_Timeout = std::numeric_limits<std::uint32_t>::max();

    constexpr std::array g_ClearValues {
            VkClearValue {
                    .color = {
                            {0.25F,
                             0.25F,
                             0.5F,
                             1.F}}},
            VkClearValue {.depthStencil = {1.0F, 0U}}};

    constexpr float g_KeyCallbackRate    = g_FrameRate / 5000.f;
    constexpr float g_CursorCallbackRate = g_FrameRate / 25000.f;
    constexpr float g_ScrollCallbackRate = g_FrameRate / 500.f;
}// namespace RenderCore
