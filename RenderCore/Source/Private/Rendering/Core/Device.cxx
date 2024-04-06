// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <GLFW/glfw3.h>
#include <Volk/volk.h>
#include <format>
#include <optional>
#include <ranges>
#include <algorithm>
#include <unordered_map>

module RenderCore.Runtime.Device;

import RenderCore.Runtime.SwapChain;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import RenderCore.Utils.DebugHelpers;

using namespace RenderCore;

VkPhysicalDevice                 g_PhysicalDevice {VK_NULL_HANDLE};
VkDevice                         g_Device {VK_NULL_HANDLE};
std::pair<std::uint8_t, VkQueue> g_GraphicsQueue {};
std::pair<std::uint8_t, VkQueue> g_PresentationQueue {};
std::pair<std::uint8_t, VkQueue> g_ComputeQueue {};
std::vector<std::uint8_t>        g_UniqueQueueFamilyIndices {};

bool IsPhysicalDeviceSuitable(VkPhysicalDevice const &Device)
{
    if (Device == VK_NULL_HANDLE)
    {
        return false;
    }

    VkPhysicalDeviceProperties SurfaceProperties;
    vkGetPhysicalDeviceProperties(Device, &SurfaceProperties);

    VkPhysicalDeviceFeatures SupportedFeatures;
    vkGetPhysicalDeviceFeatures(Device, &SupportedFeatures);

    return SurfaceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && SupportedFeatures.samplerAnisotropy != 0U;
}

bool GetQueueFamilyIndices(VkSurfaceKHR const          &VulkanSurface,
                           std::optional<std::uint8_t> &GraphicsQueueFamilyIndex,
                           std::optional<std::uint8_t> &PresentationQueueFamilyIndex,
                           std::optional<std::uint8_t> &ComputeQueueFamilyIndex)
{
    std::uint32_t QueueFamilyCount = 0U;
    vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &QueueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &QueueFamilyCount, std::data(QueueFamilies));

    for (std::uint32_t Iterator = 0U; Iterator < QueueFamilyCount; ++Iterator)
    {
        if (!GraphicsQueueFamilyIndex.has_value() && (QueueFamilies.at(Iterator).queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U)
        {
            GraphicsQueueFamilyIndex.emplace(static_cast<std::uint8_t>(Iterator));

            if (!PresentationQueueFamilyIndex.has_value())
            {
                VkBool32 PresentationSupport = 0U;
                CheckVulkanResult(vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, Iterator, VulkanSurface, &PresentationSupport));

                if (PresentationSupport != 0U)
                {
                    PresentationQueueFamilyIndex.emplace(static_cast<std::uint8_t>(Iterator));
                }
            }
        }
        else if (!ComputeQueueFamilyIndex.has_value() && (QueueFamilies.at(Iterator).queueFlags & VK_QUEUE_COMPUTE_BIT) != 0U)
        {
            ComputeQueueFamilyIndex.emplace(static_cast<std::uint8_t>(Iterator));
        }

        if (GraphicsQueueFamilyIndex.has_value() && PresentationQueueFamilyIndex.has_value() && ComputeQueueFamilyIndex.has_value())
        {
            break;
        }
    }

    return GraphicsQueueFamilyIndex.has_value() && PresentationQueueFamilyIndex.has_value() && ComputeQueueFamilyIndex.has_value();
}

void PickPhysicalDevice()
{
    for (VkPhysicalDevice const &Device : GetAvailablePhysicalDevices())
    {
        if (IsPhysicalDeviceSuitable(Device))
        {
            g_PhysicalDevice = Device;
            break;
        }
    }
}

void CreateLogicalDevice(VkSurfaceKHR const &VulkanSurface)
{
    std::optional<std::uint8_t> GraphicsQueueFamilyIndex {std::nullopt};
    std::optional<std::uint8_t> ComputeQueueFamilyIndex {std::nullopt};
    std::optional<std::uint8_t> PresentationQueueFamilyIndex {std::nullopt};

    GetQueueFamilyIndices(VulkanSurface, GraphicsQueueFamilyIndex, PresentationQueueFamilyIndex, ComputeQueueFamilyIndex);

    if (g_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    std::vector Layers(std::cbegin(g_RequiredDeviceLayers), std::cend(g_RequiredDeviceLayers));
    std::vector Extensions(std::cbegin(g_RequiredDeviceExtensions), std::cend(g_RequiredDeviceExtensions));

#ifdef _DEBUG
    Layers.insert(std::cend(Layers), std::cbegin(g_DebugDeviceLayers), std::cend(g_DebugDeviceLayers));
    Extensions.insert(std::cend(Extensions), std::cbegin(g_DebugDeviceExtensions), std::cend(g_DebugDeviceExtensions));
#endif

    auto const AvailableLayers = GetAvailablePhysicalDeviceLayersNames();
    GetAvailableResources("device layers", Layers, g_OptionalDeviceLayers, AvailableLayers);

    auto const AvailableExtensions = GetAvailablePhysicalDeviceExtensionsNames();
    GetAvailableResources("device extensions", Extensions, g_OptionalDeviceExtensions, AvailableExtensions);

    std::unordered_map<std::uint8_t, std::uint8_t> QueueFamilyIndices {{g_GraphicsQueue.first, 1U}};

    if (!QueueFamilyIndices.contains(g_ComputeQueue.first))
    {
        QueueFamilyIndices.emplace(g_ComputeQueue.first, 1U);
    }
    else
    {
        ++QueueFamilyIndices.at(g_ComputeQueue.first);
    }

    g_UniqueQueueFamilyIndices.clear();
    g_UniqueQueueFamilyIndices.reserve(std::size(QueueFamilyIndices));

    std::vector<VkDeviceQueueCreateInfo> QueueCreateInfo;
    QueueCreateInfo.reserve(std::size(QueueFamilyIndices));

    std::vector Priorities {0.F};
    for (const auto &Index : QueueFamilyIndices | std::views::keys)
    {
        g_UniqueQueueFamilyIndices.push_back(Index);

        QueueCreateInfo.push_back(VkDeviceQueueCreateInfo {.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                                           .queueFamilyIndex = Index,
                                                           .queueCount       = 1U,
                                                           .pQueuePriorities = std::data(Priorities)});
    }

    VkPhysicalDeviceSynchronization2Features Synchronization2Features {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
                                                                       .pNext = nullptr,
                                                                       .synchronization2
                                                                       = Contains(AvailableExtensions, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)};

    VkPhysicalDeviceDynamicRenderingFeatures DynamicRenderingFeatures {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
                                                                       .pNext = &Synchronization2Features,
                                                                       .dynamicRendering
                                                                       = Contains(AvailableExtensions, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)};

    VkPhysicalDeviceRobustness2FeaturesEXT RobustnessFeatures {.sType          = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT,
                                                               .pNext          = &DynamicRenderingFeatures,
                                                               .nullDescriptor = Contains(AvailableExtensions, VK_EXT_PIPELINE_ROBUSTNESS_EXTENSION_NAME)};

    VkPhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT UnusedAttachmentsFeatures {
        .sType                             = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_FEATURES_EXT,
        .pNext                             = &RobustnessFeatures,
        .dynamicRenderingUnusedAttachments = Contains(AvailableExtensions, VK_EXT_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_EXTENSION_NAME)};

    VkPhysicalDeviceFeatures2 DeviceFeatures {.sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                                              .pNext    = &UnusedAttachmentsFeatures,
                                              .features = VkPhysicalDeviceFeatures {.independentBlend               = VK_TRUE,
                                                                                    .drawIndirectFirstInstance      = true,
                                                                                    .fillModeNonSolid               = true,
                                                                                    .wideLines                      = true,
                                                                                    .samplerAnisotropy              = VK_TRUE,
                                                                                    .pipelineStatisticsQuery        = true,
                                                                                    .vertexPipelineStoresAndAtomics = true,
                                                                                    .fragmentStoresAndAtomics       = true,
                                                                                    .shaderImageGatherExtended      = true,
                                                                                    .shaderInt16                    = false}};

    VkDeviceCreateInfo const DeviceCreateInfo {.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                               .pNext                   = &DeviceFeatures,
                                               .queueCreateInfoCount    = static_cast<std::uint32_t>(std::size(QueueCreateInfo)),
                                               .pQueueCreateInfos       = std::data(QueueCreateInfo),
                                               .enabledLayerCount       = static_cast<std::uint32_t>(std::size(Layers)),
                                               .ppEnabledLayerNames     = std::data(Layers),
                                               .enabledExtensionCount   = static_cast<std::uint32_t>(std::size(Extensions)),
                                               .ppEnabledExtensionNames = std::data(Extensions)};

    CheckVulkanResult(vkCreateDevice(g_PhysicalDevice, &DeviceCreateInfo, nullptr, &g_Device));
    volkLoadDevice(g_Device);

    vkGetDeviceQueue(g_Device, g_GraphicsQueue.first, 0U, &g_GraphicsQueue.second);
    vkGetDeviceQueue(g_Device, g_PresentationQueue.first, 0U, &g_PresentationQueue.second);
    vkGetDeviceQueue(g_Device, g_ComputeQueue.first, 0U, &g_ComputeQueue.second);
}

void RenderCore::InitializeDevice(VkSurfaceKHR const &VulkanSurface)
{
    if (g_PhysicalDevice != VK_NULL_HANDLE)
    {
        return;
    }

    PickPhysicalDevice();
    CreateLogicalDevice(VulkanSurface);
}

VkSurfaceCapabilitiesKHR RenderCore::GetSurfaceCapabilities()
{
    VkSurfaceCapabilitiesKHR Output;
    CheckVulkanResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_PhysicalDevice, GetSurface(), &Output));

    return Output;
}

SurfaceProperties RenderCore::GetSurfaceProperties(GLFWwindow *const Window)
{
    std::vector<VkSurfaceFormatKHR> const SupportedFormats           = GetAvailablePhysicalDeviceSurfaceFormats();
    std::vector<VkPresentModeKHR> const   SupportedPresentationModes = GetAvailablePhysicalDeviceSurfacePresentationModes();

    SurfaceProperties Output {.Format = SupportedFormats.front(), .Extent = GetWindowExtent(Window, GetSurfaceCapabilities())};

    if (auto const MatchingFormat = std::ranges::find_if(SupportedFormats,
                                                         [](VkSurfaceFormatKHR const &Iter)
                                                         {
                                                             return Iter.format == VK_FORMAT_R8G8B8A8_SRGB
                                                                 && Iter.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                                                         });
        MatchingFormat != std::cend(SupportedFormats))
    {
        Output.Format = *MatchingFormat;
    }

    Output.Mode = VK_PRESENT_MODE_FIFO_KHR;

    for (constexpr std::array PreferredDepthFormats = {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT};
         VkFormat const      &FormatIter : PreferredDepthFormats)
    {
        VkFormatProperties FormatProperties;
        vkGetPhysicalDeviceFormatProperties(g_PhysicalDevice, FormatIter, &FormatProperties);

        if ((FormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0U)
        {
            Output.DepthFormat = FormatIter;
            break;
        }
    }

    return Output;
}

VkDevice &RenderCore::GetLogicalDevice()
{
    return g_Device;
}

VkPhysicalDevice &RenderCore::GetPhysicalDevice()
{
    return g_PhysicalDevice;
}

std::pair<std::uint8_t, VkQueue> &RenderCore::GetGraphicsQueue()
{
    return g_GraphicsQueue;
}

std::pair<std::uint8_t, VkQueue> &RenderCore::GetComputeQueue()
{
    return g_ComputeQueue;
}

std::pair<std::uint8_t, VkQueue> &RenderCore::GetPresentationQueue()
{
    return g_PresentationQueue;
}

std::vector<std::uint32_t> RenderCore::GetUniqueQueueFamilyIndicesU32()
{
    std::vector<std::uint32_t> QueueFamilyIndicesU32(std::size(g_UniqueQueueFamilyIndices));

    std::ranges::transform(g_UniqueQueueFamilyIndices,
                           std::begin(QueueFamilyIndicesU32),
                           [](std::uint8_t const &Index)
                           {
                               return static_cast<std::uint32_t>(Index);
                           });

    return QueueFamilyIndicesU32;
}

void RenderCore::ReleaseDeviceResources()
{
    vkDestroyDevice(g_Device, nullptr);
    g_Device = VK_NULL_HANDLE;

    g_PhysicalDevice           = VK_NULL_HANDLE;
    g_GraphicsQueue.second     = VK_NULL_HANDLE;
    g_PresentationQueue.second = VK_NULL_HANDLE;
}

std::vector<VkPhysicalDevice> RenderCore::GetAvailablePhysicalDevices()
{
    VkInstance const &VulkanInstance = volkGetLoadedInstance();

    std::uint32_t DeviceCount = 0U;
    CheckVulkanResult(vkEnumeratePhysicalDevices(VulkanInstance, &DeviceCount, nullptr));

    std::vector<VkPhysicalDevice> Output(DeviceCount, VK_NULL_HANDLE);
    CheckVulkanResult(vkEnumeratePhysicalDevices(VulkanInstance, &DeviceCount, std::data(Output)));

    return Output;
}

std::vector<VkExtensionProperties> RenderCore::GetAvailablePhysicalDeviceExtensions()
{
    std::uint32_t ExtensionsCount = 0;
    CheckVulkanResult(vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &ExtensionsCount, nullptr));

    std::vector<VkExtensionProperties> Output(ExtensionsCount);
    CheckVulkanResult(vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &ExtensionsCount, std::data(Output)));

    return Output;
}

std::vector<VkLayerProperties> RenderCore::GetAvailablePhysicalDeviceLayers()
{
    std::uint32_t LayersCount = 0;
    CheckVulkanResult(vkEnumerateDeviceLayerProperties(g_PhysicalDevice, &LayersCount, nullptr));

    std::vector<VkLayerProperties> Output(LayersCount);
    CheckVulkanResult(vkEnumerateDeviceLayerProperties(g_PhysicalDevice, &LayersCount, std::data(Output)));

    return Output;
}

std::vector<VkExtensionProperties> RenderCore::GetAvailablePhysicalDeviceLayerExtensions(std::string_view const  LayerName)
{
    if (std::vector<std::string> const AvailableLayers = GetAvailablePhysicalDeviceLayersNames();
        std::ranges::find(AvailableLayers, LayerName) == std::cend(AvailableLayers))
    {
        return {};
    }

    std::uint32_t ExtensionsCount = 0;
    CheckVulkanResult(vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, std::data(LayerName), &ExtensionsCount, nullptr));

    std::vector<VkExtensionProperties> Output(ExtensionsCount);
    CheckVulkanResult(vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, std::data(LayerName), &ExtensionsCount, std::data(Output)));

    return Output;
}

std::vector<std::string> RenderCore::GetAvailablePhysicalDeviceExtensionsNames()
{
    std::vector<std::string> Output;
    for (const auto &[Name, Version] : GetAvailablePhysicalDeviceExtensions())
    {
        Output.emplace_back(Name);
    }

    return Output;
}

std::vector<std::string> RenderCore::GetAvailablePhysicalDeviceLayerExtensionsNames(std::string_view const LayerName)
{
    std::vector<std::string> Output;
    for (const auto &[Name, Version] : GetAvailablePhysicalDeviceLayerExtensions(LayerName))
    {
        Output.emplace_back(Name);
    }

    return Output;
}

std::vector<std::string> RenderCore::GetAvailablePhysicalDeviceLayersNames()
{
    std::vector<std::string> Output;
    for (const auto &[Name, SpecVer, ImplVer, Description] : GetAvailablePhysicalDeviceLayers())
    {
        Output.emplace_back(Name);
    }

    return Output;
}

std::vector<VkSurfaceFormatKHR> RenderCore::GetAvailablePhysicalDeviceSurfaceFormats()
{
    std::uint32_t Count = 0U;
    CheckVulkanResult(vkGetPhysicalDeviceSurfaceFormatsKHR(g_PhysicalDevice, GetSurface(), &Count, nullptr));

    std::vector Output(Count, VkSurfaceFormatKHR());
    CheckVulkanResult(vkGetPhysicalDeviceSurfaceFormatsKHR(g_PhysicalDevice, GetSurface(), &Count, std::data(Output)));

    return Output;
}

std::vector<VkPresentModeKHR> RenderCore::GetAvailablePhysicalDeviceSurfacePresentationModes()
{
    std::uint32_t Count = 0U;
    CheckVulkanResult(vkGetPhysicalDeviceSurfacePresentModesKHR(g_PhysicalDevice, GetSurface(), &Count, nullptr));

    std::vector Output(Count, VkPresentModeKHR());
    CheckVulkanResult(vkGetPhysicalDeviceSurfacePresentModesKHR(g_PhysicalDevice, GetSurface(), &Count, std::data(Output)));

    return Output;
}

VkDeviceSize RenderCore::GetMinUniformBufferOffsetAlignment()
{
    VkPhysicalDeviceProperties SurfaceProperties;
    vkGetPhysicalDeviceProperties(g_PhysicalDevice, &SurfaceProperties);

    return SurfaceProperties.limits.minUniformBufferOffsetAlignment;
}
