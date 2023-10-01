// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

#include "Utils/RenderCoreHelpers.h"
#include "Managers/VulkanBufferManager.h"
#include "Managers/VulkanDeviceManager.h"
#include <GLFW/glfw3.h>
#include <boost/log/trivial.hpp>
#include <chrono>
#include <glm/glm.hpp>
#include <numbers>
#include <span>

using namespace RenderCore;

std::vector<char const*> RenderCoreHelpers::GetGLFWExtensions()
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

VkExtent2D RenderCoreHelpers::GetWindowExtent(GLFWwindow* const Window, VkSurfaceCapabilitiesKHR const& Capabilities)
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

std::vector<VkLayerProperties> RenderCoreHelpers::GetAvailableInstanceLayers()
{
    std::uint32_t LayersCount = 0U;
    CheckVulkanResult(vkEnumerateInstanceLayerProperties(&LayersCount, nullptr));

    std::vector Output(LayersCount, VkLayerProperties());
    CheckVulkanResult(vkEnumerateInstanceLayerProperties(&LayersCount, Output.data()));

    return Output;
}

std::vector<std::string> RenderCoreHelpers::GetAvailableInstanceLayersNames()
{
    std::vector<std::string> Output;
    for (auto const& [LayerName, SpecVer, ImplVer, Descr]: GetAvailableInstanceLayers())
    {
        Output.emplace_back(LayerName);
    }

    return Output;
}

std::vector<VkExtensionProperties> RenderCoreHelpers::GetAvailableInstanceExtensions()
{
    std::uint32_t ExtensionCount = 0U;
    CheckVulkanResult(vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr));

    std::vector Output(ExtensionCount, VkExtensionProperties());
    CheckVulkanResult(vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, Output.data()));

    return Output;
}

std::vector<std::string> RenderCoreHelpers::GetAvailableInstanceExtensionsNames()
{
    std::vector<std::string> Output;
    for (auto const& [ExtName, SpecVer]: GetAvailableInstanceExtensions())
    {
        Output.emplace_back(ExtName);
    }

    return Output;
}

#ifdef _DEBUG

void RenderCoreHelpers::ListAvailableInstanceLayers()
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

void RenderCoreHelpers::ListAvailableInstanceExtensions()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available instance extensions...";

    for (auto const& [ExtName, SpecVer]: GetAvailableInstanceExtensions())
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Extension Name: " << ExtName;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Extension Spec Version: " << SpecVer << std::endl;
    }
}

#endif

std::vector<VkExtensionProperties> RenderCoreHelpers::GetAvailableLayerExtensions(std::string_view const LayerName)
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

std::vector<std::string> RenderCoreHelpers::GetAvailableLayerExtensionsNames(std::string_view const LayerName)
{
    std::vector<std::string> Output;
    for (auto const& [ExtName, SpecVer]: GetAvailableLayerExtensions(LayerName))
    {
        Output.emplace_back(ExtName);
    }

    return Output;
}

#ifdef _DEBUG

void RenderCoreHelpers::ListAvailableInstanceLayerExtensions(std::string_view const LayerName)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available layer '" << LayerName << "' extensions...";

    for (auto const& [ExtName, SpecVer]: GetAvailableLayerExtensions(LayerName))
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Extension Name: " << ExtName;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Extension Spec Version: " << SpecVer << std::endl;
    }
}

#endif

void RenderCoreHelpers::InitializeSingleCommandQueue(VkCommandPool& CommandPool, VkCommandBuffer& CommandBuffer, std::uint8_t const QueueFamilyIndex)
{
    VkDevice const& VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

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

void RenderCoreHelpers::FinishSingleCommandQueue(VkQueue const& Queue, VkCommandPool const& CommandPool, VkCommandBuffer const& CommandBuffer)
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

    VkDevice const& VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

    vkFreeCommandBuffers(VulkanLogicalDevice, CommandPool, 1U, &CommandBuffer);
    vkDestroyCommandPool(VulkanLogicalDevice, CommandPool, nullptr);
}

UniformBufferObject RenderCoreHelpers::GetUniformBufferObject()
{
    auto const& [Width, Height] = VulkanBufferManager::Get().GetSwapChainExtent();

    static auto StartTime  = std::chrono::high_resolution_clock::now();
    auto const CurrentTime = std::chrono::high_resolution_clock::now();
    float const Time       = std::chrono::duration<float>(CurrentTime - StartTime).count();

    glm::mat4 const Model = rotate(glm::mat4(1.F), Time * glm::radians(90.F), glm::vec3(0.F, 0.F, 1.F));
    glm::mat4 const View  = lookAt(glm::vec3(2.F, 2.F, 2.F), glm::vec3(0.F, 0.F, 0.F), glm::vec3(0.F, 0.F, 1.F));
    glm::mat4 Projection  = glm::perspective(glm::radians(45.0F), static_cast<float>(Width) / static_cast<float>(Height), 0.1F, 10.F);
    Projection[1][1] *= -1;

    return UniformBufferObject {
            .ModelViewProjection = Projection * View * Model};
}
