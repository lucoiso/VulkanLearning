// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <Volk/volk.h>
#include <boost/log/trivial.hpp>
#include <filesystem>
#include <span>

#ifndef GLM_FORCE_RADIANS
    #define GLM_FORCE_RADIANS
#endif
#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
    #define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif
#include <glm/ext.hpp>

module RenderCore.Utils.Helpers;

import RenderCore.Types.Vertex;
import RuntimeInfo.Manager;

using namespace RenderCore;

VkExtent2D RenderCore::GetWindowExtent(GLFWwindow *const Window, VkSurfaceCapabilitiesKHR const &Capabilities)
{
    std::int32_t Width  = 0U;
    std::int32_t Height = 0U;
    glfwGetFramebufferSize(Window, &Width, &Height);

    VkExtent2D ActualExtent {.width = static_cast<std::uint32_t>(Width), .height = static_cast<std::uint32_t>(Height)};

    ActualExtent.width  = std::clamp(ActualExtent.width, Capabilities.minImageExtent.width, Capabilities.maxImageExtent.width);
    ActualExtent.height = std::clamp(ActualExtent.height, Capabilities.minImageExtent.height, Capabilities.maxImageExtent.height);

    return ActualExtent;
}

std::vector<std::string> RenderCore::GetGLFWExtensions()
{
    auto const _ {RuntimeInfo::Manager::Get().PushCallstackWithCounter()};
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Getting GLFW extensions";

    std::uint32_t   GLFWExtensionsCount = 0U;
    char const    **GLFWExtensions      = glfwGetRequiredInstanceExtensions(&GLFWExtensionsCount);
    std::span const GLFWExtensionsSpan(GLFWExtensions, GLFWExtensionsCount);

    std::vector<std::string> Output {};
    Output.reserve(GLFWExtensionsCount);

    if (!std::empty(GLFWExtensionsSpan))
    {
        BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Found extensions:";

        for (char const *const &ExtensionIter : GLFWExtensionsSpan)
        {
            BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: " << ExtensionIter;
            Output.emplace_back(ExtensionIter);
        }
    }
    else
    {
        BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Failed to get GLFW extensions. Forcing Vulkan extensions:";
        BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: VK_KHR_surface";
        Output.emplace_back("VK_KHR_surface");

#ifdef WIN32
        BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: VK_KHR_win32_surface";
        Output.emplace_back("VK_KHR_win32_surface");
#elif __linux__
        BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: VK_KHR_xcb_surface";
        Output.emplace_back("VK_KHR_xcb_surface");
#elif __APPLE__
        BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: VK_KHR_macos_surface";
        Output.emplace_back("VK_KHR_macos_surface");
        elif __ANDROID__ BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: VK_KHR_android_surface";
        Output.emplace_back("VK_KHR_android_surface");
#endif
    }

    return Output;
}

std::vector<VkLayerProperties> RenderCore::GetAvailableInstanceLayers()
{
    auto const _ {RuntimeInfo::Manager::Get().PushCallstackWithCounter()};

    std::uint32_t LayersCount = 0U;
    CheckVulkanResult(vkEnumerateInstanceLayerProperties(&LayersCount, nullptr));

    std::vector Output(LayersCount, VkLayerProperties());
    CheckVulkanResult(vkEnumerateInstanceLayerProperties(&LayersCount, std::data(Output)));

    return Output;
}

std::vector<std::string> RenderCore::GetAvailableInstanceLayersNames()
{
    auto const _ {RuntimeInfo::Manager::Get().PushCallstackWithCounter()};

    std::vector<std::string> Output;
    for (auto const &[LayerName, SpecVer, ImplVer, Descr] : GetAvailableInstanceLayers())
    {
        Output.emplace_back(LayerName);
    }

    return Output;
}

std::vector<VkExtensionProperties> RenderCore::GetAvailableInstanceExtensions()
{
    auto const _ {RuntimeInfo::Manager::Get().PushCallstackWithCounter()};

    std::uint32_t ExtensionCount = 0U;
    CheckVulkanResult(vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr));

    std::vector Output(ExtensionCount, VkExtensionProperties());
    CheckVulkanResult(vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, std::data(Output)));

    return Output;
}

std::vector<std::string> RenderCore::GetAvailableInstanceExtensionsNames()
{
    auto const _ {RuntimeInfo::Manager::Get().PushCallstackWithCounter()};

    std::vector<std::string> Output;
    for (auto const &[ExtName, SpecVer] : GetAvailableInstanceExtensions())
    {
        Output.emplace_back(ExtName);
    }

    return Output;
}

VkVertexInputBindingDescription RenderCore::GetBindingDescriptors(std::uint32_t const InBinding)
{
    return VkVertexInputBindingDescription {.binding = InBinding, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
}

std::vector<VkVertexInputAttributeDescription> RenderCore::GetAttributeDescriptions(std::uint32_t const                                   InBinding,
                                                                                    std::vector<VkVertexInputAttributeDescription> const &Attributes)
{
    std::vector Output(std::cbegin(Attributes), std::cend(Attributes));

    std::uint32_t AttributeLocation {0U};
    for (auto &[Location, Binding, Format, Offset] : Output)
    {
        Binding  = InBinding;
        Location = AttributeLocation;
        ++AttributeLocation;
    }

    return Output;
}

std::vector<VkExtensionProperties> RenderCore::GetAvailableInstanceLayerExtensions(std::string_view const LayerName)
{
    auto const _ {RuntimeInfo::Manager::Get().PushCallstackWithCounter()};

    if (std::vector<std::string> const AvailableLayers = GetAvailableInstanceLayersNames();
        std::ranges::find(AvailableLayers, LayerName) == std::cend(AvailableLayers))
    {
        return {};
    }

    std::uint32_t ExtensionCount = 0U;
    CheckVulkanResult(vkEnumerateInstanceExtensionProperties(std::data(LayerName), &ExtensionCount, nullptr));

    std::vector Output(ExtensionCount, VkExtensionProperties());
    CheckVulkanResult(vkEnumerateInstanceExtensionProperties(std::data(LayerName), &ExtensionCount, std::data(Output)));

    return Output;
}

std::vector<std::string> RenderCore::GetAvailableInstanceLayerExtensionsNames(std::string_view const LayerName)
{
    auto const _ {RuntimeInfo::Manager::Get().PushCallstackWithCounter()};

    std::vector<std::string> Output;
    for (auto const &[ExtName, SpecVer] : GetAvailableInstanceLayerExtensions(LayerName))
    {
        Output.emplace_back(ExtName);
    }

    return Output;
}
