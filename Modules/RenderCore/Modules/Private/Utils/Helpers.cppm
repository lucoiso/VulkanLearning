// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <GLFW/glfw3.h>
#include <boost/log/trivial.hpp>
#include <volk.h>

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif
#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/ext.hpp>

module RenderCore.Utils.Helpers;

import <algorithm>;
import <array>;
import <chrono>;
import <concepts>;
import <cstdint>;
import <string>;
import <span>;

import RenderCore.EngineCore;
import RenderCore.Management.BufferManagement;
import RenderCore.Management.DeviceManagement;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Types.Vertex;
import RenderCore.Utils.Constants;
import RenderCore.Utils.EnumConverter;

using namespace RenderCore;

std::vector<char const*> RenderCore::GetGLFWExtensions()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Getting GLFW extensions";

    std::uint32_t GLFWExtensionsCount = 0U;
    char const** const GLFWExtensions = glfwGetRequiredInstanceExtensions(&GLFWExtensionsCount);

    std::span const GLFWExtensionsSpan(GLFWExtensions, GLFWExtensionsCount);
    std::vector Output(GLFWExtensionsSpan.begin(), GLFWExtensionsSpan.end());

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Found extensions:";

    for (char const* const& ExtensionIter: Output)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: " << ExtensionIter;
    }

    return Output;
}

VkExtent2D RenderCore::GetWindowExtent(GLFWwindow* const Window, VkSurfaceCapabilitiesKHR const& Capabilities)
{
    std::int32_t Width  = 0U;
    std::int32_t Height = 0U;
    glfwGetFramebufferSize(Window, &Width, &Height);

    VkExtent2D ActualExtent {
            .width  = static_cast<std::uint32_t>(Width),
            .height = static_cast<std::uint32_t>(Height)};

    ActualExtent.width  = std::clamp(ActualExtent.width, Capabilities.minImageExtent.width, Capabilities.maxImageExtent.width);
    ActualExtent.height = std::clamp(ActualExtent.height, Capabilities.minImageExtent.height, Capabilities.maxImageExtent.height);

    return ActualExtent;
}

std::vector<VkLayerProperties> RenderCore::GetAvailableInstanceLayers()
{
    std::uint32_t LayersCount = 0U;
    CheckVulkanResult(vkEnumerateInstanceLayerProperties(&LayersCount, nullptr));

    std::vector Output(LayersCount, VkLayerProperties());
    CheckVulkanResult(vkEnumerateInstanceLayerProperties(&LayersCount, Output.data()));

    return Output;
}

std::vector<std::string> RenderCore::GetAvailableInstanceLayersNames()
{
    std::vector<std::string> Output;
    for (auto const& [LayerName, SpecVer, ImplVer, Descr]: GetAvailableInstanceLayers())
    {
        Output.emplace_back(LayerName);
    }

    return Output;
}

std::vector<VkExtensionProperties> RenderCore::GetAvailableInstanceExtensions()
{
    std::uint32_t ExtensionCount = 0U;
    CheckVulkanResult(vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr));

    std::vector Output(ExtensionCount, VkExtensionProperties());
    CheckVulkanResult(vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, Output.data()));

    return Output;
}

std::vector<std::string> RenderCore::GetAvailableInstanceExtensionsNames()
{
    std::vector<std::string> Output;
    for (auto const& [ExtName, SpecVer]: GetAvailableInstanceExtensions())
    {
        Output.emplace_back(ExtName);
    }

    return Output;
}

#ifdef _DEBUG

void RenderCore::ListAvailableInstanceLayers()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available instance layers...";

    for (auto const& [LayerName, SpecVer, ImplVer, Descr]: GetAvailableInstanceLayers())
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Name: " << LayerName;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Description: " << Descr;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Spec Version: " << SpecVer;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Implementation Version: " << ImplVer << std::endl;
    }
}

void RenderCore::ListAvailableInstanceExtensions()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available instance extensions...";

    for (auto const& [ExtName, SpecVer]: GetAvailableInstanceExtensions())
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Extension Name: " << ExtName;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Extension Spec Version: " << SpecVer << std::endl;
    }
}

#endif

std::array<VkVertexInputBindingDescription, 1U> RenderCore::GetBindingDescriptors()
{
    return {
            VkVertexInputBindingDescription {
                    .binding   = 0U,
                    .stride    = sizeof(Vertex),
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX}};
}

std::array<VkVertexInputAttributeDescription, 3U> RenderCore::GetAttributeDescriptions()
{
    return {
            VkVertexInputAttributeDescription {
                    .location = 0U,
                    .binding  = 0U,
                    .format   = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset   = static_cast<std::uint32_t>(offsetof(Vertex, Position))},
            VkVertexInputAttributeDescription {
                    .location = 1U,
                    .binding  = 0U,
                    .format   = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset   = static_cast<std::uint32_t>(offsetof(Vertex, Color))},
            VkVertexInputAttributeDescription {
                    .location = 2U,
                    .binding  = 0U,
                    .format   = VK_FORMAT_R32G32_SFLOAT,
                    .offset   = static_cast<std::uint32_t>(offsetof(Vertex, TextureCoordinate))}};
}

std::vector<VkExtensionProperties> RenderCore::GetAvailableLayerExtensions(std::string_view const LayerName)
{
    if (std::vector<std::string> const AvailableLayers = GetAvailableInstanceLayersNames();
        std::ranges::find(AvailableLayers, LayerName) == AvailableLayers.end())
    {
        return {};
    }

    std::uint32_t ExtensionCount = 0U;
    CheckVulkanResult(vkEnumerateInstanceExtensionProperties(LayerName.data(), &ExtensionCount, nullptr));

    std::vector Output(ExtensionCount, VkExtensionProperties());
    CheckVulkanResult(vkEnumerateInstanceExtensionProperties(LayerName.data(), &ExtensionCount, Output.data()));

    return Output;
}

std::vector<std::string> RenderCore::GetAvailableLayerExtensionsNames(std::string_view const LayerName)
{
    std::vector<std::string> Output;
    for (auto const& [ExtName, SpecVer]: GetAvailableLayerExtensions(LayerName))
    {
        Output.emplace_back(ExtName);
    }

    return Output;
}

#ifdef _DEBUG

void RenderCore::ListAvailableInstanceLayerExtensions(std::string_view const LayerName)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available layer '" << LayerName << "' extensions...";

    for (auto const& [ExtName, SpecVer]: GetAvailableLayerExtensions(LayerName))
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Extension Name: " << ExtName;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Extension Spec Version: " << SpecVer << std::endl;
    }
}

#endif

void RenderCore::InitializeSingleCommandQueue(VkCommandPool& CommandPool, VkCommandBuffer& CommandBuffer, std::uint8_t const QueueFamilyIndex)
{
    VkDevice const& VulkanLogicalDevice = GetLogicalDevice();

    VkCommandPoolCreateInfo const CommandPoolCreateInfo {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = static_cast<std::uint32_t>(QueueFamilyIndex)};

    CheckVulkanResult(vkCreateCommandPool(VulkanLogicalDevice, &CommandPoolCreateInfo, nullptr, &CommandPool));

    constexpr VkCommandBufferBeginInfo CommandBufferBeginInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    VkCommandBufferAllocateInfo const CommandBufferAllocateInfo {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = CommandPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1U,
    };

    CheckVulkanResult(vkAllocateCommandBuffers(VulkanLogicalDevice, &CommandBufferAllocateInfo, &CommandBuffer));
    CheckVulkanResult(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));
}

void RenderCore::FinishSingleCommandQueue(VkQueue const& Queue, VkCommandPool const& CommandPool, VkCommandBuffer const& CommandBuffer)
{
    if (CommandPool == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan command pool is invalid.");
    }

    if (CommandBuffer == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan command buffer is invalid.");
    }

    CheckVulkanResult(vkEndCommandBuffer(CommandBuffer));

    VkSubmitInfo const SubmitInfo {
            .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1U,
            .pCommandBuffers    = &CommandBuffer,
    };

    CheckVulkanResult(vkQueueSubmit(Queue, 1U, &SubmitInfo, VK_NULL_HANDLE));
    CheckVulkanResult(vkQueueWaitIdle(Queue));

    VkDevice const& VulkanLogicalDevice = GetLogicalDevice();

    vkFreeCommandBuffers(VulkanLogicalDevice, CommandPool, 1U, &CommandBuffer);
    vkDestroyCommandPool(VulkanLogicalDevice, CommandPool, nullptr);
}

UniformBufferObject RenderCore::GetUniformBufferObject()
{
    auto const& [Width, Height] = GetSwapChainExtent();

    glm::mat4 const Model = glm::mat4(1.F);
    glm::mat4 Projection  = glm::perspective(glm::radians(45.F), static_cast<float>(Width) / static_cast<float>(Height), 0.1F, 10.F);
    Projection[1][1] *= -1;

    glm::mat4 const ModelViewProjection = Projection * GetCameraMatrix() * Model;

    std::array<std::array<float, 4U>, 4U> ModelViewProjectionArray {};
    for (std::uint8_t Column = 0U; Column < static_cast<std::uint8_t>(ModelViewProjectionArray.size()); ++Column)
    {
        for (std::uint8_t Line = 0U; Line < static_cast<std::uint8_t>(ModelViewProjectionArray[Column].size()); ++Line)
        {
            ModelViewProjectionArray[Column][Line] = ModelViewProjection[Column][Line];
        }
    }

    return UniformBufferObject {
            .ModelViewProjection = ModelViewProjectionArray};
}