// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "Utils/VulkanDebugHelpers.h"
#include "Utils/VulkanConstants.h"
#include <boost/log/trivial.hpp>

using namespace RenderCore;

#ifdef _DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL DebugHelpers::ValidationLayerDebugCallback(const VkDebugUtilsMessageSeverityFlagBitsEXT      MessageSeverity,
                                                                          [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT  MessageType,
                                                                          const VkDebugUtilsMessengerCallbackDataEXT* const CallbackData,
                                                                          [[maybe_unused]] void*                            UserData)
{
    if (MessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Message: " << CallbackData->pMessage;
    }

    return VK_FALSE;
}

VkResult DebugHelpers::CreateDebugUtilsMessenger(const VkInstance                                Instance,
                                                 const VkDebugUtilsMessengerCreateInfoEXT* const CreateInfo,
                                                 const VkAllocationCallbacks* const              Allocator,
                                                 VkDebugUtilsMessengerEXT* const                 DebugMessenger)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating debug messenger";

    if (const auto CreationFunctor = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT")))
    {
        return CreationFunctor(Instance, CreateInfo, Allocator, DebugMessenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DebugHelpers::DestroyDebugUtilsMessenger(const VkInstance Instance, const VkDebugUtilsMessengerEXT DebugMessenger, const VkAllocationCallbacks* const Allocator)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying debug messenger";

    if (const auto DestructionFunctor = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT")))
    {
        DestructionFunctor(Instance, DebugMessenger, Allocator);
    }
}

VkValidationFeaturesEXT DebugHelpers::GetInstanceValidationFeatures()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Getting validation features";

    constexpr VkValidationFeaturesEXT InstanceValidationFeatures{
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .enabledValidationFeatureCount = static_cast<uint32_t>(g_EnabledInstanceValidationFeatures.size()),
        .pEnabledValidationFeatures = g_EnabledInstanceValidationFeatures.data()
    };

    return InstanceValidationFeatures;
}

void DebugHelpers::PopulateDebugInfo(VkDebugUtilsMessengerCreateInfoEXT& Info, void* const UserData)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Populating debug info";

    Info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    Info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    Info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;

    Info.pfnUserCallback = ValidationLayerDebugCallback;
    Info.pUserData       = UserData;
}
#endif
