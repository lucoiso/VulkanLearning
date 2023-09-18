// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#ifndef RENDERCOREHELPERS_H
#define RENDERCOREHELPERS_H

#pragma once

#include "Utils/VulkanConstants.h"
#include "Utils/VulkanEnumConverter.h"
#include "Types/VulkanVertex.h"
#include <QWindow>
#include <vulkan/vulkan.h>
#include <numbers>
#include <boost/log/trivial.hpp>

namespace RenderCore
{
#define RENDERCORE_CHECK_VULKAN_RESULT(INPUT_OPERATION)                                                                   \
    if (const VkResult OperationResult = INPUT_OPERATION; OperationResult != VK_SUCCESS)                                  \
    {                                                                                                                     \
        throw std::runtime_error("Vulkan operation failed with result: " + std::string(ResultToString(OperationResult))); \
    }

    static inline VkExtent2D GetWindowExtent(const QWindow *const Window, const VkSurfaceCapabilitiesKHR &Capabilities)
    {
        const std::int32_t Width = Window->width();
        const std::int32_t Height = Window->height();

        VkExtent2D ActualExtent{
            .width = static_cast<std::uint32_t>(Width),
            .height = static_cast<std::uint32_t>(Height)};

        ActualExtent.width = std::clamp(ActualExtent.width, Capabilities.minImageExtent.width, Capabilities.maxImageExtent.width);
        ActualExtent.height = std::clamp(ActualExtent.height, Capabilities.minImageExtent.height, Capabilities.maxImageExtent.height);

        return ActualExtent;
    }

    static inline std::vector<VkLayerProperties> GetAvailableInstanceLayers()
    {
        std::uint32_t LayersCount = 0u;
        RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateInstanceLayerProperties(&LayersCount, nullptr));
        
        std::vector<VkLayerProperties> Output;
        Output.resize(LayersCount);
        RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateInstanceLayerProperties(&LayersCount, Output.data()));

        return Output;
    }

    static inline std::vector<std::string> GetAvailableInstanceLayersNames()
    {        
        std::vector<std::string> Output;
        for (const VkLayerProperties &LayerIter : GetAvailableInstanceLayers())
        {
            Output.emplace_back(LayerIter.layerName);
        }

        return Output;
    }

    static inline std::vector<VkExtensionProperties> GetAvailableInstanceExtensions()
    {
        std::uint32_t ExtensionCount = 0u;
        RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr));
        
        std::vector<VkExtensionProperties> Output;
        Output.resize(ExtensionCount);
        RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, Output.data()));

        return Output;
    }

    static inline std::vector<std::string> GetAvailableInstanceExtensionsNames()
    {        
        std::vector<std::string> Output;
        for (const VkExtensionProperties &ExtensionIter : GetAvailableInstanceExtensions())
        {
            Output.emplace_back(ExtensionIter.extensionName);
        }

        return Output;
    }

#ifdef _DEBUG
    static inline void ListAvailableInstanceLayers()
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available instance layers...";

        for (const VkLayerProperties &LayerIter : GetAvailableInstanceLayers())
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Name: " << LayerIter.layerName;
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Description: " << LayerIter.description;
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Spec Version: " << LayerIter.specVersion;
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Implementation Version: " << LayerIter.implementationVersion << std::endl;
        }
    }    
    
    static inline void ListAvailableInstanceExtensions()
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available instance extensions...";

        for (const VkExtensionProperties &ExtensionIter : GetAvailableInstanceExtensions())
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Extension Name: " << ExtensionIter.extensionName;
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Extension Spec Version: " << ExtensionIter.specVersion << std::endl;
        }
    }
#endif

    static inline std::vector<VkExtensionProperties> GetAvailableLayerExtensions(const std::string_view LayerName)
    {        
        const std::vector<std::string> AvailableLayers = GetAvailableInstanceLayersNames();
        if (std::find(AvailableLayers.begin(), AvailableLayers.end(), LayerName) == AvailableLayers.end())
        {
            return std::vector<VkExtensionProperties>();
        }

        std::uint32_t ExtensionCount = 0u;
        RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateInstanceExtensionProperties(LayerName.data(), &ExtensionCount, nullptr));
        
        std::vector<VkExtensionProperties> Output;
        Output.resize(ExtensionCount);
        RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateInstanceExtensionProperties(LayerName.data(), &ExtensionCount, Output.data()));

        return Output;
    }

    static inline std::vector<std::string> GetAvailableLayerExtensionsNames(const std::string_view LayerName)
    {        
        std::vector<std::string> Output;
        for (const VkExtensionProperties &ExtensionIter : GetAvailableLayerExtensions(LayerName))
        {
            Output.emplace_back(ExtensionIter.extensionName);
        }

        return Output;
    }

#ifdef _DEBUG
    static inline void ListAvailableInstanceLayerExtensions(const std::string_view LayerName)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available layer '" << LayerName << "' extensions...";

        for (const VkExtensionProperties &ExtensionIter : GetAvailableLayerExtensions(LayerName))
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Extension Name: " << ExtensionIter.extensionName;
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Extension Spec Version: " << ExtensionIter.specVersion << std::endl;
        }
    }
#endif

    constexpr std::array<VkVertexInputBindingDescription, 1> GetBindingDescriptors()
    {
        return {
            VkVertexInputBindingDescription{
                .binding = 0u,
                .stride = sizeof(Vertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX}};
    }

    constexpr std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescriptions()
    {
        return {
            VkVertexInputAttributeDescription{
                .location = 0u,
                .binding = 0u,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, Position)},
            VkVertexInputAttributeDescription{
                .location = 1u,
                .binding = 0u,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, Normal)},
            VkVertexInputAttributeDescription{
                .location = 2u,
                .binding = 0u,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, Color)},
            VkVertexInputAttributeDescription{
                .location = 3u,
                .binding = 0u,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, TextureCoordinate)}};
    }

    template<typename T>
    constexpr void AddFlags(T &Lhs, const T Rhs)
    {
        Lhs = static_cast<T>(static_cast<std::uint8_t>(Lhs) | static_cast<std::uint8_t>(Rhs));
    }

    template<typename T>
    constexpr void RemoveFlags(T &Lhs, const T Rhs)
    {
        Lhs = static_cast<T>(static_cast<std::uint8_t>(Lhs) & ~static_cast<std::uint8_t>(Rhs));
    }
    
    template<typename T>
    constexpr bool HasFlag(const T Lhs, const T Rhs)
    {
        return (Lhs & Rhs) == Rhs;
    }
    
    template<typename T>
    constexpr bool HasAnyFlag(const T Lhs)
    {
        return static_cast<std::uint8_t>(Lhs) != 0u;
    }    

    static inline void InitializeSingleCommandQueue(VkCommandPool &CommandPool, VkCommandBuffer &CommandBuffer, const std::uint8_t QueueFamilyIndex)
    {
        const VkDevice &VulkanLogicalDevice = VulkanRenderSubsystem::Get()->GetDevice();

        const VkCommandPoolCreateInfo CommandPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = static_cast<std::uint32_t>(QueueFamilyIndex)};

        RENDERCORE_CHECK_VULKAN_RESULT(vkCreateCommandPool(VulkanLogicalDevice, &CommandPoolCreateInfo, nullptr, &CommandPool));

        const VkCommandBufferBeginInfo CommandBufferBeginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        const VkCommandBufferAllocateInfo CommandBufferAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = CommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1u,
        };

        RENDERCORE_CHECK_VULKAN_RESULT(vkAllocateCommandBuffers(VulkanLogicalDevice, &CommandBufferAllocateInfo, &CommandBuffer));
        RENDERCORE_CHECK_VULKAN_RESULT(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));        
    }

    static inline void FinishSingleCommandQueue(const VkQueue &Queue, const VkCommandPool &CommandPool, const VkCommandBuffer &CommandBuffer)
    {
        if (CommandPool == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan command pool is invalid.");
        }

        if (CommandBuffer == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan command buffer is invalid.");
        }

        RENDERCORE_CHECK_VULKAN_RESULT(vkEndCommandBuffer(CommandBuffer));

        const VkSubmitInfo SubmitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1u,
            .pCommandBuffers = &CommandBuffer,
        };

        RENDERCORE_CHECK_VULKAN_RESULT(vkQueueSubmit(Queue, 1u, &SubmitInfo, VK_NULL_HANDLE));
        RENDERCORE_CHECK_VULKAN_RESULT(vkQueueWaitIdle(Queue));

        const VkDevice &VulkanLogicalDevice = VulkanRenderSubsystem::Get()->GetDevice();

        vkFreeCommandBuffers(VulkanLogicalDevice, CommandPool, 1u, &CommandBuffer);
        vkDestroyCommandPool(VulkanLogicalDevice, CommandPool, nullptr);
    }
}
#endif