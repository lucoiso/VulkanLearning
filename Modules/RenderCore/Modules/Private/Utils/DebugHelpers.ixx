// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#pragma once

#include <volk.h>

export module RenderCore.Utils.DebugHelpers;

namespace RenderCore
{
#ifdef _DEBUG
    VKAPI_ATTR VkBool32 VKAPI_CALL ValidationLayerDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT,
                                                                VkDebugUtilsMessageTypeFlagsEXT,
                                                                VkDebugUtilsMessengerCallbackDataEXT const*,
                                                                void*);

    export VkResult CreateDebugUtilsMessenger(VkInstance,
                                              VkDebugUtilsMessengerCreateInfoEXT const*,
                                              VkAllocationCallbacks const*,
                                              VkDebugUtilsMessengerEXT*);

    export void DestroyDebugUtilsMessenger(VkInstance, VkDebugUtilsMessengerEXT, VkAllocationCallbacks const*);

    export VkValidationFeaturesEXT GetInstanceValidationFeatures();

    export void PopulateDebugInfo(VkDebugUtilsMessengerCreateInfoEXT&, void*);
#endif
}// namespace RenderCore