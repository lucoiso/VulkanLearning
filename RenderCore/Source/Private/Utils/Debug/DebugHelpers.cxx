// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.Utils.DebugHelpers;

using namespace RenderCore;

#ifdef _DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL RenderCore::ValidationLayerDebugCallback([[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT const MessageSeverity,
                                                                        [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT const        MessageType,
                                                                        VkDebugUtilsMessengerCallbackDataEXT const *const             CallbackData,
                                                                        [[maybe_unused]] void *                                       UserData)
{
    // if (MessageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) // - uncomment to print only warnings and errors
    // if (!HasFlag<VkDebugUtilsMessageTypeFlagsEXT>(MessageType, VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)) // - uncomment to avoid imgui performance warnings
    {
        BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Message: " << CallbackData->pMessage;
    }

    return VK_FALSE;
}

VkResult RenderCore::CreateDebugUtilsMessenger(VkInstance const                                Instance,
                                               VkDebugUtilsMessengerCreateInfoEXT const *const CreateInfo,
                                               VkAllocationCallbacks const *const              Allocator,
                                               VkDebugUtilsMessengerEXT *const                 DebugMessenger)
{
    if (auto const CreationFunctor = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(Instance,
        "vkCreateDebugUtilsMessengerEXT")))
    {
        return CreationFunctor(Instance, CreateInfo, Allocator, DebugMessenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void RenderCore::DestroyDebugUtilsMessenger(VkInstance const                   Instance,
                                            VkDebugUtilsMessengerEXT const     DebugMessenger,
                                            VkAllocationCallbacks const *const Allocator)
{
    if (auto const DestructionFunctor = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(Instance,
        "vkDestroyDebugUtilsMessengerEXT")))
    {
        DestructionFunctor(Instance, DebugMessenger, Allocator);
    }
}

VkValidationFeaturesEXT RenderCore::GetInstanceValidationFeatures()
{
    constexpr VkValidationFeaturesEXT InstanceValidationFeatures {
            .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
            .enabledValidationFeatureCount = static_cast<uint32_t>(std::size(g_DebugInstanceValidationFeatures)),
            .pEnabledValidationFeatures = std::data(g_DebugInstanceValidationFeatures)
    };

    return InstanceValidationFeatures;
}

void RenderCore::PopulateDebugInfo(VkDebugUtilsMessengerCreateInfoEXT &Info, void *const UserData)
{
    Info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    Info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    Info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;

    Info.pfnUserCallback = ValidationLayerDebugCallback;
    Info.pUserData       = UserData;
}

#endif
