// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

#include "Utils/RenderCoreHelpers.h"
#include "Managers/VulkanDeviceManager.h"
#include "Managers/VulkanBufferManager.h"
#include "VulkanRenderCore.h"
#include <glm/glm.hpp>
#include <numbers>
#include <boost/log/trivial.hpp>
#include <chrono>
#include <GLFW/glfw3.h>

using namespace RenderCore;

std::vector<const char*> RenderCoreHelpers::GetGLFWExtensions()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Getting GLFW extensions";

    std::uint32_t      GLFWExtensionsCount = 0u;
    const char** const GLFWExtensions      = glfwGetRequiredInstanceExtensions(&GLFWExtensionsCount);

    const std::vector Output(GLFWExtensions, GLFWExtensions + GLFWExtensionsCount);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Found extensions:";

    for (const char* const & ExtensionIter : Output)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: " << ExtensionIter;
    }

    return Output;
}

VkExtent2D RenderCoreHelpers::GetWindowExtent(GLFWwindow* const Window, const VkSurfaceCapabilitiesKHR& Capabilities)
{
    std::int32_t Width  = 0u;
    std::int32_t Height = 0u;
    glfwGetFramebufferSize(Window, &Width, &Height);

    VkExtent2D ActualExtent{.width = static_cast<std::uint32_t>(Width), .height = static_cast<std::uint32_t>(Height)};

    ActualExtent.width  = std::clamp(ActualExtent.width, Capabilities.minImageExtent.width, Capabilities.maxImageExtent.width);
    ActualExtent.height = std::clamp(ActualExtent.height, Capabilities.minImageExtent.height, Capabilities.maxImageExtent.height);

    return ActualExtent;
}

std::vector<VkLayerProperties> RenderCoreHelpers::GetAvailableInstanceLayers()
{
    std::uint32_t LayersCount = 0u;
    RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateInstanceLayerProperties(&LayersCount, nullptr));

    std::vector Output(LayersCount, VkLayerProperties());
    RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateInstanceLayerProperties(&LayersCount, Output.data()));

    return Output;
}

std::vector<std::string> RenderCoreHelpers::GetAvailableInstanceLayersNames()
{
    std::vector<std::string> Output;
    for (const auto& [LayerName, SpecVer, ImplVer, Descr] : GetAvailableInstanceLayers())
    {
        Output.emplace_back(LayerName);
    }

    return Output;
}

std::vector<VkExtensionProperties> RenderCoreHelpers::GetAvailableInstanceExtensions()
{
    std::uint32_t ExtensionCount = 0u;
    RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr));

    std::vector Output(ExtensionCount, VkExtensionProperties());
    RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, Output.data()));

    return Output;
}

std::vector<std::string> RenderCoreHelpers::GetAvailableInstanceExtensionsNames()
{
    std::vector<std::string> Output;
    for (const auto& [ExtName, SpecVer] : GetAvailableInstanceExtensions())
    {
        Output.emplace_back(ExtName);
    }

    return Output;
}

#ifdef _DEBUG
void RenderCoreHelpers::ListAvailableInstanceLayers()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available instance layers...";

    for (const auto& [LayerName, SpecVer, ImplVer, Descr] : GetAvailableInstanceLayers())
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

    for (const auto& [ExtName, SpecVer] : GetAvailableInstanceExtensions())
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Extension Name: " << ExtName;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Extension Spec Version: " << SpecVer << std::endl;
    }
}
#endif

std::vector<VkExtensionProperties> RenderCoreHelpers::GetAvailableLayerExtensions(const std::string_view LayerName)
{
    if (const std::vector<std::string> AvailableLayers = GetAvailableInstanceLayersNames();
        std::ranges::find(AvailableLayers, LayerName) == AvailableLayers.end())
    {
        return std::vector<VkExtensionProperties>();
    }

    std::uint32_t ExtensionCount = 0u;
    RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateInstanceExtensionProperties(LayerName.data(), &ExtensionCount, nullptr));

    std::vector Output(ExtensionCount, VkExtensionProperties());
    RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateInstanceExtensionProperties(LayerName.data(), &ExtensionCount, Output.data()));

    return Output;
}

std::vector<std::string> RenderCoreHelpers::GetAvailableLayerExtensionsNames(const std::string_view LayerName)
{
    std::vector<std::string> Output;
    for (const auto& [ExtName, SpecVer] : GetAvailableLayerExtensions(LayerName))
    {
        Output.emplace_back(ExtName);
    }

    return Output;
}

#ifdef _DEBUG
void RenderCoreHelpers::ListAvailableInstanceLayerExtensions(const std::string_view LayerName)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available layer '" << LayerName << "' extensions...";

    for (const auto& [ExtName, SpecVer] : GetAvailableLayerExtensions(LayerName))
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Extension Name: " << ExtName;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Extension Spec Version: " << SpecVer << std::endl;
    }
}
#endif

void RenderCoreHelpers::InitializeSingleCommandQueue(VkCommandPool& CommandPool, VkCommandBuffer& CommandBuffer, const std::uint8_t QueueFamilyIndex)
{
    const VkDevice& VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

    const VkCommandPoolCreateInfo CommandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = static_cast<std::uint32_t>(QueueFamilyIndex)
    };

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateCommandPool(VulkanLogicalDevice, &CommandPoolCreateInfo, nullptr, &CommandPool));

    constexpr VkCommandBufferBeginInfo CommandBufferBeginInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,};

    const VkCommandBufferAllocateInfo CommandBufferAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = CommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1u,
    };

    RENDERCORE_CHECK_VULKAN_RESULT(vkAllocateCommandBuffers(VulkanLogicalDevice, &CommandBufferAllocateInfo, &CommandBuffer));
    RENDERCORE_CHECK_VULKAN_RESULT(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));
}

void RenderCoreHelpers::FinishSingleCommandQueue(const VkQueue& Queue, const VkCommandPool& CommandPool, const VkCommandBuffer& CommandBuffer)
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

    const VkSubmitInfo SubmitInfo{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1u, .pCommandBuffers = &CommandBuffer,};

    RENDERCORE_CHECK_VULKAN_RESULT(vkQueueSubmit(Queue, 1u, &SubmitInfo, VK_NULL_HANDLE));
    RENDERCORE_CHECK_VULKAN_RESULT(vkQueueWaitIdle(Queue));

    const VkDevice& VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

    vkFreeCommandBuffers(VulkanLogicalDevice, CommandPool, 1u, &CommandBuffer);
    vkDestroyCommandPool(VulkanLogicalDevice, CommandPool, nullptr);
}

UniformBufferObject RenderCoreHelpers::GetUniformBufferObject()
{
    const auto& [Width, Height] = VulkanBufferManager::Get().GetSwapChainExtent();

    static auto StartTime   = std::chrono::high_resolution_clock::now();
    const auto  CurrentTime = std::chrono::high_resolution_clock::now();
    const float Time        = std::chrono::duration<float>(CurrentTime - StartTime).count();

    const glm::mat4 Model      = rotate(glm::mat4(1.f), Time * glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f));
    const glm::mat4 View       = lookAt(glm::vec3(2.f, 2.f, 2.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 1.f));
    glm::mat4       Projection = glm::perspective(glm::radians(45.0f), Width / static_cast<float>(Height), 0.1f, 10.f);
    Projection[1][1] *= -1;

    return UniformBufferObject{.ModelViewProjection = Projection * View * Model};
}
