// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;


#ifdef _DEBUG
#include <Volk/volk.h>
#include <boost/log/trivial.hpp>
#endif

module RenderCore.Utils.DebugHelpers;

import RenderCore.Utils.Constants;
import RuntimeInfo.Manager;

using namespace RenderCore;

#ifdef _DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL RenderCore::ValidationLayerDebugCallback([[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT const MessageSeverity,
                                                                        [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT              MessageType,
                                                                        VkDebugUtilsMessengerCallbackDataEXT const *const             CallbackData,
                                                                        [[maybe_unused]] void *                                       UserData)
{
    RuntimeInfo::Manager::Get().PushCallstack();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Message: " << CallbackData->pMessage;
    return VK_FALSE;
}

VkResult RenderCore::CreateDebugUtilsMessenger(VkInstance const                                Instance,
                                               VkDebugUtilsMessengerCreateInfoEXT const *const CreateInfo,
                                               VkAllocationCallbacks const *const              Allocator,
                                               VkDebugUtilsMessengerEXT *const                 DebugMessenger)
{
    RuntimeInfo::Manager::Get().PushCallstack();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating debug messenger";

    if (auto const CreationFunctor = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT")))
    {
        return CreationFunctor(Instance, CreateInfo, Allocator, DebugMessenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void RenderCore::DestroyDebugUtilsMessenger(VkInstance const                   Instance,
                                            VkDebugUtilsMessengerEXT const     DebugMessenger,
                                            VkAllocationCallbacks const *const Allocator)
{
    RuntimeInfo::Manager::Get().PushCallstack();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Destroying debug messenger";

    if (auto const DestructionFunctor = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT")))
    {
        DestructionFunctor(Instance, DebugMessenger, Allocator);
    }
}

VkValidationFeaturesEXT RenderCore::GetInstanceValidationFeatures()
{
    RuntimeInfo::Manager::Get().PushCallstack();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Getting validation features";

    constexpr VkValidationFeaturesEXT InstanceValidationFeatures{
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .enabledValidationFeatureCount = static_cast<uint32_t>(std::size(g_EnabledInstanceValidationFeatures)),
        .pEnabledValidationFeatures = std::data(g_EnabledInstanceValidationFeatures)
    };

    return InstanceValidationFeatures;
}

void RenderCore::PopulateDebugInfo(VkDebugUtilsMessengerCreateInfoEXT& Info,
                                   void *const                         UserData)
{
    RuntimeInfo::Manager::Get().PushCallstack();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Populating debug info";

    Info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    Info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    Info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;

    Info.pfnUserCallback = ValidationLayerDebugCallback;
    Info.pUserData       = UserData;
}

#endif
