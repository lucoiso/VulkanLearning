// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#ifdef _DEBUG
    #include <Volk/volk.h>
    #include <array>
    #include <string_view>
#endif

export module RenderCore.Utils.DebugHelpers;

namespace RenderCore
{
#ifdef _DEBUG
    #if defined(GPU_API_DUMP) && GPU_API_DUMP
    export constexpr std::array g_DebugInstanceLayers = {"VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_api_dump"};

    export constexpr std::array g_DebugDeviceLayers = {"VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_api_dump"};
    #else
    export constexpr std::array g_DebugInstanceLayers = {"VK_LAYER_KHRONOS_validation"};

    export constexpr std::array g_DebugDeviceLayers = {"VK_LAYER_KHRONOS_validation"};
    #endif

    export constexpr std::array g_DebugInstanceExtensions = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME};

    export constexpr std::array<const char *, 0U> g_DebugDeviceExtensions = {
        // VK_AMD_BUFFER_MARKER_EXTENSION_NAME
        // VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME
    };

    export constexpr std::array g_DebugInstanceValidationFeatures = {
        // VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
        // VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
        VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
        VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT};

    VKAPI_ATTR VkBool32 VKAPI_CALL ValidationLayerDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT,
                                                                VkDebugUtilsMessageTypeFlagsEXT,
                                                                VkDebugUtilsMessengerCallbackDataEXT const *,
                                                                void *);

    export VkResult
    CreateDebugUtilsMessenger(VkInstance, VkDebugUtilsMessengerCreateInfoEXT const *, VkAllocationCallbacks const *, VkDebugUtilsMessengerEXT *);

    export void DestroyDebugUtilsMessenger(VkInstance, VkDebugUtilsMessengerEXT, VkAllocationCallbacks const *);

    export VkValidationFeaturesEXT GetInstanceValidationFeatures();

    export void PopulateDebugInfo(VkDebugUtilsMessengerCreateInfoEXT &, void *);
#endif
} // namespace RenderCore
