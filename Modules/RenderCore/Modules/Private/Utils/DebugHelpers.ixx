// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <volk.h>

export module RenderCore.Utils.DebugHelpers;

export namespace RenderCore
{
#ifdef _DEBUG
    VKAPI_ATTR VkBool32 VKAPI_CALL ValidationLayerDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT,
                                                                VkDebugUtilsMessageTypeFlagsEXT,
                                                                VkDebugUtilsMessengerCallbackDataEXT const*,
                                                                void*);

    VkResult CreateDebugUtilsMessenger(VkInstance,
                                       VkDebugUtilsMessengerCreateInfoEXT const*,
                                       VkAllocationCallbacks const*,
                                       VkDebugUtilsMessengerEXT*);

    void DestroyDebugUtilsMessenger(VkInstance, VkDebugUtilsMessengerEXT, VkAllocationCallbacks const*);

    VkValidationFeaturesEXT GetInstanceValidationFeatures();

    void PopulateDebugInfo(VkDebugUtilsMessengerCreateInfoEXT&, void*);
#endif
}// namespace RenderCore