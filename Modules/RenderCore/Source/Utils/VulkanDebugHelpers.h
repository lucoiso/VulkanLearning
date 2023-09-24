// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include <volk.h>

namespace RenderCore
{
#ifdef _DEBUG
    class DebugHelpers
    {
    public:
        static VKAPI_ATTR VkBool32 VKAPI_CALL ValidationLayerDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
                                                                           VkDebugUtilsMessageTypeFlagsEXT MessageType,
                                                                           const VkDebugUtilsMessengerCallbackDataEXT *const CallbackData,
                                                                           void *UserData);

        static VkResult CreateDebugUtilsMessenger(VkInstance Instance,
                                                  const VkDebugUtilsMessengerCreateInfoEXT *const CreateInfo,
                                                  const VkAllocationCallbacks *const Allocator,
                                                  VkDebugUtilsMessengerEXT *const DebugMessenger);

        static void DestroyDebugUtilsMessenger(VkInstance Instance,
                                               VkDebugUtilsMessengerEXT DebugMessenger,
                                               const VkAllocationCallbacks *const Allocator);

        static VkValidationFeaturesEXT GetInstanceValidationFeatures();

        static void PopulateDebugInfo(VkDebugUtilsMessengerCreateInfoEXT &Info, void *const UserData = nullptr);
    };
#endif
}