// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#ifndef VULKANDEBUGHELPERS_H
#define VULKANDEBUGHELPERS_H

#pragma once

#include <vulkan/vulkan.h>
#include <boost/log/trivial.hpp>

namespace RenderCore
{
    static inline VKAPI_ATTR VkBool32 VKAPI_CALL ValidationLayerDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
                                                                              [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT MessageType,
                                                                              const VkDebugUtilsMessengerCallbackDataEXT *const CallbackData,
                                                                              [[maybe_unused]] void *UserData)
    {
#ifdef NDEBUG
        if (MessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
#endif
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Message: " << CallbackData->pMessage;
        }

        return VK_FALSE;
    }

    static inline VkResult CreateDebugUtilsMessenger(VkInstance Instance,
                                                     const VkDebugUtilsMessengerCreateInfoEXT *const CreateInfo,
                                                     const VkAllocationCallbacks *const Allocator,
                                                     VkDebugUtilsMessengerEXT *const DebugMessenger)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating debug messenger";

        if (const auto CreationFunctor = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT")))
        {
            return CreationFunctor(Instance, CreateInfo, Allocator, DebugMessenger);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    static inline void DestroyDebugUtilsMessenger(VkInstance Instance,
                                                  VkDebugUtilsMessengerEXT DebugMessenger,
                                                  const VkAllocationCallbacks *const Allocator)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying debug messenger";

        if (const auto DestructionFunctor = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT")))
        {
            DestructionFunctor(Instance, DebugMessenger, Allocator);
        }
    }

    static inline void PopulateDebugInfo(VkDebugUtilsMessengerCreateInfoEXT &Info, void *const UserData = nullptr)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Populating debug info";

        Info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

        Info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        Info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        Info.pfnUserCallback = ValidationLayerDebugCallback;
        Info.pUserData = UserData;
    }
}

#endif