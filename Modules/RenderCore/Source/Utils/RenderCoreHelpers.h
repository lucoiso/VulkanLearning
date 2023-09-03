// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#ifndef RENDERCOREHELPERS_H
#define RENDERCOREHELPERS_H

#pragma once

#include "Utils/VulkanConstants.h"
#include "Utils/VulkanEnumConverter.h"
#include <boost/log/trivial.hpp>
#include <vulkan/vulkan.h>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

namespace RenderCore
{
    static inline std::vector<const char*> GetGLFWExtensions()
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Getting GLFW extensions";

        std::uint32_t GLFWExtensionsCount = 0u;
        const char** const GLFWExtensions = glfwGetRequiredInstanceExtensions(&GLFWExtensionsCount);

        const std::vector<const char*> Output(GLFWExtensions, GLFWExtensions + GLFWExtensionsCount);

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Found extensions:";

        for (const char* const& ExtensionIter : Output)
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: " << ExtensionIter;
        }

        return Output;
    }

    static inline VkExtent2D GetWindowExtent(GLFWwindow* const Window, const VkSurfaceCapabilitiesKHR& Capabilities)
    {
        std::int32_t Width = 0u;
        std::int32_t Height = 0u;
        glfwGetFramebufferSize(Window, &Width, &Height);

        VkExtent2D ActualExtent{
            .width = static_cast<std::uint32_t>(Width),
            .height = static_cast<std::uint32_t>(Height)
        };

        ActualExtent.width = std::clamp(ActualExtent.width, Capabilities.minImageExtent.width, Capabilities.maxImageExtent.width);
        ActualExtent.height = std::clamp(ActualExtent.height, Capabilities.minImageExtent.height, Capabilities.maxImageExtent.height);

        return ActualExtent;
    }

    static inline std::vector<VkLayerProperties> GetAvailableValidationLayers()
    {
#ifdef NDEBUG
        return {};
#else
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Getting available validation layers";

        std::vector<VkLayerProperties> Output;

        if (g_ValidationLayers.empty())
        {
            return Output;
        }

        std::uint32_t LayersCount = 0u;
        if (vkEnumerateInstanceLayerProperties(&LayersCount, nullptr) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to enumerate Vulkan Layers.");
        }

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Found " << LayersCount << " validation layers";

        if (LayersCount == 0u)
        {
            return Output;
        }

        Output.resize(LayersCount);
        if (vkEnumerateInstanceLayerProperties(&LayersCount, Output.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to get Vulkan Layers properties.");
        }

        for (const VkLayerProperties& LayerIter : Output)
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Name: " << LayerIter.layerName;
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Description: " << LayerIter.description;
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Spec Version: " << LayerIter.specVersion;
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Implementation Version: " << LayerIter.implementationVersion << std::endl;
        }

        return Output;
#endif
    }
}

#define RENDERCORE_CHECK_VULKAN_RESULT(INPUT_OPERATION) \
if (const VkResult OperationResult = INPUT_OPERATION; OperationResult != VK_SUCCESS) \
{ \
    throw std::runtime_error("Vulkan operation failed with result: " + std::string(ResultToString(OperationResult))); \
}

#endif