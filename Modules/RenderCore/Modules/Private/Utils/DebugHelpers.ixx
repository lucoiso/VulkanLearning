// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <volk.h>

export module RenderCore.Utils.DebugHelpers;

export namespace RenderCore
{
#ifdef _DEBUG
    class DebugHelpers
    {
    public:
        static VKAPI_ATTR VkBool32 VKAPI_CALL ValidationLayerDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
                                                                           VkDebugUtilsMessageTypeFlagsEXT MessageType,
                                                                           VkDebugUtilsMessengerCallbackDataEXT const* CallbackData,
                                                                           void* UserData);

        static VkResult CreateDebugUtilsMessenger(VkInstance Instance,
                                                  VkDebugUtilsMessengerCreateInfoEXT const* CreateInfo,
                                                  VkAllocationCallbacks const* Allocator,
                                                  VkDebugUtilsMessengerEXT* DebugMessenger);

        static void DestroyDebugUtilsMessenger(VkInstance Instance, VkDebugUtilsMessengerEXT DebugMessenger, VkAllocationCallbacks const* Allocator);

        static VkValidationFeaturesEXT GetInstanceValidationFeatures();

        static void PopulateDebugInfo(VkDebugUtilsMessengerCreateInfoEXT& Info, void* UserData = nullptr);
    };
#endif
}// namespace RenderCore