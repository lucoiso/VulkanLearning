// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

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

import <span>;
import <array>;
import <filesystem>;

import RenderCore.Types.Vertex;
import Timer.ExecutionCounter;

using namespace RenderCore;

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

std::vector<char const*> RenderCore::GetGLFWExtensions()
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Getting GLFW extensions";

    std::uint32_t GLFWExtensionsCount = 0U;
    char const** GLFWExtensions       = glfwGetRequiredInstanceExtensions(&GLFWExtensionsCount);

    std::vector<char const*> Output(GLFWExtensionsCount, nullptr);
    std::move(GLFWExtensions, GLFWExtensions + GLFWExtensionsCount, std::data(Output));

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Found extensions:";

    for (char const* const& ExtensionIter: Output)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: " << ExtensionIter;
    }

    return Output;
}

std::vector<VkLayerProperties> RenderCore::GetAvailableInstanceLayers()
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    std::uint32_t LayersCount = 0U;
    CheckVulkanResult(vkEnumerateInstanceLayerProperties(&LayersCount, nullptr));

    std::vector Output(LayersCount, VkLayerProperties());
    CheckVulkanResult(vkEnumerateInstanceLayerProperties(&LayersCount, std::data(Output)));

    return Output;
}

std::vector<std::string> RenderCore::GetAvailableInstanceLayersNames()
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    std::vector<std::string> Output;
    for (auto const& [LayerName, SpecVer, ImplVer, Descr]: GetAvailableInstanceLayers())
    {
        Output.emplace_back(LayerName);
    }

    return Output;
}

std::vector<VkExtensionProperties> RenderCore::GetAvailableInstanceExtensions()
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    std::uint32_t ExtensionCount = 0U;
    CheckVulkanResult(vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, nullptr));

    std::vector Output(ExtensionCount, VkExtensionProperties());
    CheckVulkanResult(vkEnumerateInstanceExtensionProperties(nullptr, &ExtensionCount, std::data(Output)));

    return Output;
}

std::vector<std::string> RenderCore::GetAvailableInstanceExtensionsNames()
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    std::vector<std::string> Output;
    for (auto const& [ExtName, SpecVer]: GetAvailableInstanceExtensions())
    {
        Output.emplace_back(ExtName);
    }

    return Output;
}

std::unordered_map<std::string, std::string> RenderCore::GetAvailableglTFAssetsInDirectory(std::string_view const& Root, std::vector<std::string_view> const& Extensions)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    std::unordered_map<std::string, std::string> OptionsMap {{"None", ""}};
    try
    {
        for (auto const& Entry: std::filesystem::recursive_directory_iterator(Root))
        {
            if (Entry.is_regular_file() && std::ranges::find(Extensions, Entry.path().extension()) != std::cend(Extensions))
            {
                OptionsMap.emplace(Entry.path().stem().string(), Entry.path().string());
            }
        }
    }
    catch (...)
    {
    }

    return OptionsMap;
}

std::array<VkVertexInputBindingDescription, 1U> RenderCore::GetBindingDescriptors()
{
    return {
            VkVertexInputBindingDescription {
                    .binding   = 0U,
                    .stride    = sizeof(Vertex),
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX}};
}

std::array<VkVertexInputAttributeDescription, 4U> RenderCore::GetAttributeDescriptions()
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
                    .offset   = static_cast<std::uint32_t>(offsetof(Vertex, Normal))},
            VkVertexInputAttributeDescription {
                    .location = 2U,
                    .binding  = 0U,
                    .format   = VK_FORMAT_R32G32B32A32_SFLOAT,
                    .offset   = static_cast<std::uint32_t>(offsetof(Vertex, Color))},
            VkVertexInputAttributeDescription {
                    .location = 3U,
                    .binding  = 0U,
                    .format   = VK_FORMAT_R32G32_SFLOAT,
                    .offset   = static_cast<std::uint32_t>(offsetof(Vertex, TextureCoordinate))}};
}

std::vector<VkExtensionProperties> RenderCore::GetAvailableLayerExtensions(std::string_view const& LayerName)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

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

std::vector<std::string> RenderCore::GetAvailableLayerExtensionsNames(std::string_view const& LayerName)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    std::vector<std::string> Output;
    for (auto const& [ExtName, SpecVer]: GetAvailableLayerExtensions(LayerName))
    {
        Output.emplace_back(ExtName);
    }

    return Output;
}