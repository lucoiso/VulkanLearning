// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <boost/log/trivial.hpp>
#include <volk.h>

module RenderCore.Management.DeviceManagement;

import <optional>;
import <string_view>;
import <vector>;
import <cstdint>;
import <unordered_map>;

import RenderCore.EngineCore;
import RenderCore.Utils.Constants;
import RenderCore.Utils.Helpers;
import RenderCore.Types.DeviceProperties;
import RenderCore.Utils.EnumConverter;

using namespace RenderCore;

VkPhysicalDevice g_PhysicalDevice {VK_NULL_HANDLE};
VkDevice g_Device {VK_NULL_HANDLE};
DeviceProperties g_DeviceProperties {};
std::pair<std::uint8_t, VkQueue> g_GraphicsQueue {};
std::pair<std::uint8_t, VkQueue> g_PresentationQueue {};
std::pair<std::uint8_t, VkQueue> g_TransferQueue {};
std::vector<std::uint8_t> g_UniqueQueueFamilyIndices {};

bool GetQueueFamilyIndices(std::optional<std::uint8_t>& GraphicsQueueFamilyIndex,
                           std::optional<std::uint8_t>& PresentationQueueFamilyIndex,
                           std::optional<std::uint8_t>& TransferQueueFamilyIndex)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Getting queue family indices";

    if (g_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    VkSurfaceKHR const& VulkanSurface = GetSurface();

    std::uint32_t QueueFamilyCount = 0U;
    vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &QueueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &QueueFamilyCount, QueueFamilies.data());

    for (std::uint32_t Iterator = 0U; Iterator < QueueFamilyCount; ++Iterator)
    {
        if (!GraphicsQueueFamilyIndex.has_value() && (QueueFamilies.at(Iterator).queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U)
        {
            GraphicsQueueFamilyIndex = static_cast<std::uint8_t>(Iterator);
        }
        else if (!TransferQueueFamilyIndex.has_value() && (QueueFamilies.at(Iterator).queueFlags & VK_QUEUE_TRANSFER_BIT) != 0U)
        {
            TransferQueueFamilyIndex = static_cast<std::uint8_t>(Iterator);
        }
        else if (!PresentationQueueFamilyIndex.has_value())
        {
            VkBool32 PresentationSupport = 0U;
            CheckVulkanResult(vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, Iterator, VulkanSurface, &PresentationSupport));

            if (PresentationSupport != 0U)
            {
                PresentationQueueFamilyIndex = static_cast<std::uint8_t>(Iterator);
            }
        }

        if (GraphicsQueueFamilyIndex.has_value() && PresentationQueueFamilyIndex.has_value() && TransferQueueFamilyIndex.has_value())
        {
            break;
        }
    }

    return GraphicsQueueFamilyIndex.has_value() && PresentationQueueFamilyIndex.has_value() && TransferQueueFamilyIndex.has_value();
}

#ifdef _DEBUG

void ListAvailablePhysicalDevices()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available vulkan physical devices...";

    for (VkPhysicalDevice const& DeviceIter: GetAvailablePhysicalDevices())
    {
        VkPhysicalDeviceProperties DeviceProperties;
        vkGetPhysicalDeviceProperties(DeviceIter, &DeviceProperties);

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Name: " << DeviceProperties.deviceName;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target ID: " << DeviceProperties.deviceID;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Vendor ID: " << DeviceProperties.vendorID;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Driver Version: " << DeviceProperties.driverVersion;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target API Version: " << DeviceProperties.apiVersion << std::endl;
    }
}

void ListAvailablePhysicalDeviceLayers()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available vulkan physical device layers...";

    for (auto const& [LayerName, SpecVer, ImplVer, Descr]: GetAvailablePhysicalDeviceLayers())
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Name: " << LayerName;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Spec Version: " << SpecVer;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Implementation Version: " << ImplVer;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Description: " << Descr << std::endl;
    }
}

void ListAvailablePhysicalDeviceLayerExtensions(std::string_view const LayerName)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available vulkan physical device layer '" << LayerName << "' extensions...";

    for (auto const& [ExtName, SpecVer]: GetAvailablePhysicalDeviceLayerExtensions(LayerName))
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Name: " << ExtName;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Spec Version: " << SpecVer << std::endl;
    }
}

void ListAvailablePhysicalDeviceSurfaceCapabilities()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available vulkan physical device surface capabilities...";

    VkSurfaceCapabilitiesKHR const SurfaceCapabilities = GetAvailablePhysicalDeviceSurfaceCapabilities();

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Min ImageAllocation Count: " << SurfaceCapabilities.minImageCount;
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Max ImageAllocation Count: " << SurfaceCapabilities.maxImageCount;
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Current Extent: (" << SurfaceCapabilities.currentExtent.width << ", " << SurfaceCapabilities.currentExtent.height << ")";
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Min ImageAllocation Extent: (" << SurfaceCapabilities.minImageExtent.width << ", " << SurfaceCapabilities.minImageExtent.height << ")";
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Max ImageAllocation Extent: (" << SurfaceCapabilities.maxImageExtent.width << ", " << SurfaceCapabilities.maxImageExtent.height << ")";
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Max ImageAllocation Array Layers: " << SurfaceCapabilities.maxImageArrayLayers;
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Supported Transforms: " << TransformFlagToString(static_cast<VkSurfaceTransformFlagBitsKHR>(SurfaceCapabilities.supportedTransforms));
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Current Transform: " << TransformFlagToString(SurfaceCapabilities.currentTransform);
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Supported Composite Alpha: " << CompositeAlphaFlagToString(SurfaceCapabilities.supportedCompositeAlpha);
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Supported Usage Flags: " << ImageUsageFlagToString(static_cast<VkImageUsageFlagBits>(SurfaceCapabilities.supportedUsageFlags));
}

void ListAvailablePhysicalDeviceSurfaceFormats()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available vulkan physical device surface formats...";

    for (auto const& [Format, ColorSpace]: GetAvailablePhysicalDeviceSurfaceFormats())
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Format: " << SurfaceFormatToString(Format);
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Color Space: " << ColorSpaceModeToString(ColorSpace) << std::endl;
    }
}

void ListAvailablePhysicalDeviceSurfacePresentationModes()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available vulkan physical device presentation modes...";

    for (VkPresentModeKHR const& FormatIter: GetAvailablePhysicalDeviceSurfacePresentationModes())
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Mode: " << PresentationModeToString(FormatIter);
    }
}
#endif

bool IsPhysicalDeviceSuitable(VkPhysicalDevice const& Device)
{
    if (Device == VK_NULL_HANDLE)
    {
        return false;
    }

    VkPhysicalDeviceProperties DeviceProperties;
    vkGetPhysicalDeviceProperties(Device, &DeviceProperties);

    VkPhysicalDeviceFeatures SupportedFeatures;
    vkGetPhysicalDeviceFeatures(Device, &SupportedFeatures);

    return DeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && SupportedFeatures.samplerAnisotropy != 0U;
}

void RenderCore::PickPhysicalDevice()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Picking a physical device";

    for (VkPhysicalDevice const& DeviceIter: GetAvailablePhysicalDevices())
    {
        if (g_PhysicalDevice == VK_NULL_HANDLE && IsPhysicalDeviceSuitable(DeviceIter))
        {
            g_PhysicalDevice = DeviceIter;
            break;
        }
    }

    if (g_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("No suitable Vulkan physical device found.");
    }

#ifdef _DEBUG
    ListAvailablePhysicalDevices();
    ListAvailablePhysicalDeviceLayers();

    for (char const* const& RequiredLayerIter: g_RequiredDeviceLayers)
    {
        ListAvailablePhysicalDeviceLayerExtensions(RequiredLayerIter);
    }

    for (char const* const& DebugLayerIter: g_DebugDeviceLayers)
    {
        ListAvailablePhysicalDeviceLayerExtensions(DebugLayerIter);
    }

    ListAvailablePhysicalDeviceSurfaceCapabilities();
    ListAvailablePhysicalDeviceSurfaceFormats();
    ListAvailablePhysicalDeviceSurfacePresentationModes();
#endif
}

void RenderCore::CreateLogicalDevice()
{
    std::optional<std::uint8_t> GraphicsQueueFamilyIndex     = std::nullopt;
    std::optional<std::uint8_t> PresentationQueueFamilyIndex = std::nullopt;
    std::optional<std::uint8_t> TransferQueueFamilyIndex     = std::nullopt;

    if (!GetQueueFamilyIndices(GraphicsQueueFamilyIndex, PresentationQueueFamilyIndex, TransferQueueFamilyIndex))
    {
        throw std::runtime_error("Failed to get queue family indices.");
    }

    g_GraphicsQueue.first     = GraphicsQueueFamilyIndex.value();
    g_PresentationQueue.first = PresentationQueueFamilyIndex.value();
    g_TransferQueue.first     = TransferQueueFamilyIndex.value();

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan logical device";

    if (g_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    std::vector Layers(g_RequiredDeviceLayers.begin(), g_RequiredDeviceLayers.end());
    std::vector Extensions(g_RequiredDeviceExtensions.begin(), g_RequiredDeviceExtensions.end());

#ifdef _DEBUG
    Layers.insert(Layers.end(), g_DebugDeviceLayers.begin(), g_DebugDeviceLayers.end());
    Extensions.insert(Extensions.end(), g_DebugDeviceExtensions.begin(), g_DebugDeviceExtensions.end());
#endif

    std::unordered_map<std::uint8_t, std::uint8_t> QueueFamilyIndices;
    QueueFamilyIndices.emplace(g_GraphicsQueue.first, 1U);
    if (!QueueFamilyIndices.contains(g_PresentationQueue.first))
    {
        QueueFamilyIndices.emplace(g_PresentationQueue.first, 1U);
    }
    else
    {
        ++QueueFamilyIndices.at(g_PresentationQueue.first);
    }

    if (!QueueFamilyIndices.contains(g_TransferQueue.first))
    {
        QueueFamilyIndices.emplace(g_TransferQueue.first, 1U);
    }
    else
    {
        ++QueueFamilyIndices.at(g_TransferQueue.first);
    }

    g_UniqueQueueFamilyIndices.clear();
    std::unordered_map<std::uint32_t, std::vector<float>> QueuePriorities;
    for (auto const& [Index, Quantity]: QueueFamilyIndices)
    {
        g_UniqueQueueFamilyIndices.push_back(Index);
        QueuePriorities.emplace(Index, std::vector(Quantity, 1.0F));
    }

    std::vector<VkDeviceQueueCreateInfo> QueueCreateInfo;
    for (auto const& [Index, Count]: QueueFamilyIndices)
    {
        QueueCreateInfo.push_back(
                VkDeviceQueueCreateInfo {
                        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                        .queueFamilyIndex = Index,
                        .queueCount       = Count,
                        .pQueuePriorities = QueuePriorities.at(Index).data()});
    }

    VkPhysicalDeviceRobustness2FeaturesEXT RobustnessFeatures {
            .sType          = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT,
            .nullDescriptor = VK_TRUE};

    VkPhysicalDeviceFeatures2 DeviceFeatures {
            .sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext    = &RobustnessFeatures,
            .features = VkPhysicalDeviceFeatures {.samplerAnisotropy = VK_TRUE}};

    VkDeviceCreateInfo const DeviceCreateInfo {
            .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext                   = &DeviceFeatures,
            .queueCreateInfoCount    = static_cast<std::uint32_t>(QueueCreateInfo.size()),
            .pQueueCreateInfos       = QueueCreateInfo.data(),
            .enabledLayerCount       = static_cast<std::uint32_t>(Layers.size()),
            .ppEnabledLayerNames     = Layers.data(),
            .enabledExtensionCount   = static_cast<std::uint32_t>(Extensions.size()),
            .ppEnabledExtensionNames = Extensions.data(),
            .pEnabledFeatures        = nullptr};

    CheckVulkanResult(vkCreateDevice(g_PhysicalDevice, &DeviceCreateInfo, nullptr, &g_Device));

    if (vkGetDeviceQueue(g_Device, g_GraphicsQueue.first, 0U, &g_GraphicsQueue.second);
        g_GraphicsQueue.second == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to get graphics queue.");
    }

    if (vkGetDeviceQueue(g_Device, g_PresentationQueue.first, 0U, &g_PresentationQueue.second);
        g_PresentationQueue.second == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to get presentation queue.");
    }

    if (vkGetDeviceQueue(g_Device, g_TransferQueue.first, 0U, &g_TransferQueue.second);
        g_TransferQueue.second == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to get transfer queue.");
    }
}

bool RenderCore::UpdateDeviceProperties(GLFWwindow* const Window)
{
    GetDeviceProperties().Capabilities = GetAvailablePhysicalDeviceSurfaceCapabilities();

    std::vector<VkSurfaceFormatKHR> const SupportedFormats = GetAvailablePhysicalDeviceSurfaceFormats();
    if (SupportedFormats.empty())
    {
        throw std::runtime_error("No supported surface formats found.");
    }

    std::vector<VkPresentModeKHR> const SupportedPresentationModes = GetAvailablePhysicalDeviceSurfacePresentationModes();
    if (SupportedFormats.empty())
    {
        throw std::runtime_error("No supported presentation modes found.");
    }

    if (GetDeviceProperties().Capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max())
    {
        GetDeviceProperties().Extent = GetDeviceProperties().Capabilities.currentExtent;
    }
    else
    {
        GetDeviceProperties().Extent = GetWindowExtent(Window, GetDeviceProperties().Capabilities);
    }

    GetDeviceProperties().Format = SupportedFormats.at(0);
    if (auto const MatchingFormat = std::ranges::find_if(
                SupportedFormats,
                [](VkSurfaceFormatKHR const& Iter) {
                    return Iter.format == VK_FORMAT_B8G8R8A8_SRGB && Iter.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                });
        MatchingFormat != SupportedFormats.end())
    {
        GetDeviceProperties().Format = *MatchingFormat;
    }

    GetDeviceProperties().Mode = VK_PRESENT_MODE_FIFO_KHR;
    if (auto const MatchingMode = std::ranges::find_if(
                SupportedPresentationModes,
                [](VkPresentModeKHR const& Iter) {
                    return Iter == VK_PRESENT_MODE_MAILBOX_KHR;
                });
        MatchingMode != SupportedPresentationModes.end())
    {
        GetDeviceProperties().Mode = *MatchingMode;
    }

    for (std::vector const PreferredDepthFormats = {
                 VK_FORMAT_D32_SFLOAT,
                 VK_FORMAT_D32_SFLOAT_S8_UINT,
                 VK_FORMAT_D24_UNORM_S8_UINT};
         VkFormat const& FormatIter: PreferredDepthFormats)
    {
        VkFormatProperties FormatProperties;
        vkGetPhysicalDeviceFormatProperties(g_PhysicalDevice, FormatIter, &FormatProperties);

        if ((FormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0U)
        {
            GetDeviceProperties().DepthFormat = FormatIter;
            break;
        }
    }

    return GetDeviceProperties().IsValid();
}

DeviceProperties& RenderCore::GetDeviceProperties()
{
    return g_DeviceProperties;
}

VkDevice& RenderCore::GetLogicalDevice()
{
    return g_Device;
}

VkPhysicalDevice& RenderCore::GetPhysicalDevice()
{
    return g_PhysicalDevice;
}

std::pair<std::uint8_t, VkQueue>& RenderCore::GetGraphicsQueue()
{
    return g_GraphicsQueue;
}

std::pair<std::uint8_t, VkQueue>& RenderCore::GetPresentationQueue()
{
    return g_PresentationQueue;
}

std::pair<std::uint8_t, VkQueue>& RenderCore::GetTransferQueue()
{
    return g_TransferQueue;
}

std::vector<std::uint8_t>& RenderCore::GetUniqueQueueFamilyIndices()
{
    return g_UniqueQueueFamilyIndices;
}

std::vector<std::uint32_t> RenderCore::GetUniqueQueueFamilyIndicesU32()
{
    std::vector<std::uint32_t> QueueFamilyIndicesU32(g_UniqueQueueFamilyIndices.size());
    std::ranges::transform(
            g_UniqueQueueFamilyIndices,
            QueueFamilyIndicesU32.begin(),
            [](std::uint8_t const& Index) {
                return static_cast<std::uint32_t>(Index);
            });
    return QueueFamilyIndicesU32;
}

std::uint32_t RenderCore::GetMinImageCount()
{
    bool const SupportsTripleBuffering = GetDeviceProperties().Capabilities.minImageCount < 3U && GetDeviceProperties().Capabilities.maxImageCount >= 3U;
    return SupportsTripleBuffering ? 3U : GetDeviceProperties().Capabilities.minImageCount;
}

void RenderCore::ReleaseDeviceResources()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Releasing vulkan device resources";

    vkDestroyDevice(g_Device, nullptr);
    g_Device = VK_NULL_HANDLE;

    g_PhysicalDevice           = VK_NULL_HANDLE;
    g_GraphicsQueue.second     = VK_NULL_HANDLE;
    g_PresentationQueue.second = VK_NULL_HANDLE;
    g_TransferQueue.second     = VK_NULL_HANDLE;
}

std::vector<VkPhysicalDevice> RenderCore::GetAvailablePhysicalDevices()
{
    VkInstance const& VulkanInstance = GetInstance();

    std::uint32_t DeviceCount = 0U;
    CheckVulkanResult(vkEnumeratePhysicalDevices(VulkanInstance, &DeviceCount, nullptr));

    std::vector<VkPhysicalDevice> Output(DeviceCount, VK_NULL_HANDLE);
    CheckVulkanResult(vkEnumeratePhysicalDevices(VulkanInstance, &DeviceCount, Output.data()));

    return Output;
}

std::vector<VkExtensionProperties> RenderCore::GetAvailablePhysicalDeviceExtensions()
{
    if (g_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    std::uint32_t ExtensionsCount = 0;
    CheckVulkanResult(vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &ExtensionsCount, nullptr));

    std::vector<VkExtensionProperties> Output(ExtensionsCount);
    CheckVulkanResult(vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &ExtensionsCount, Output.data()));

    return Output;
}

std::vector<VkLayerProperties> RenderCore::GetAvailablePhysicalDeviceLayers()
{
    if (g_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    std::uint32_t LayersCount = 0;
    CheckVulkanResult(vkEnumerateDeviceLayerProperties(g_PhysicalDevice, &LayersCount, nullptr));

    std::vector<VkLayerProperties> Output(LayersCount);
    CheckVulkanResult(vkEnumerateDeviceLayerProperties(g_PhysicalDevice, &LayersCount, Output.data()));

    return Output;
}

std::vector<VkExtensionProperties> RenderCore::GetAvailablePhysicalDeviceLayerExtensions(std::string_view const LayerName)
{
    if (g_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    if (std::vector<std::string> const AvailableLayers = GetAvailablePhysicalDeviceLayersNames();
        std::ranges::find(AvailableLayers, LayerName) == AvailableLayers.end())
    {
        return {};
    }

    std::uint32_t ExtensionsCount = 0;
    CheckVulkanResult(vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, LayerName.data(), &ExtensionsCount, nullptr));

    std::vector<VkExtensionProperties> Output(ExtensionsCount);
    CheckVulkanResult(vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, LayerName.data(), &ExtensionsCount, Output.data()));

    return Output;
}

std::vector<std::string> RenderCore::GetAvailablePhysicalDeviceExtensionsNames()
{
    std::vector<std::string> Output;
    for (VkExtensionProperties const& ExtensionIter: GetAvailablePhysicalDeviceExtensions())
    {
        Output.emplace_back(ExtensionIter.extensionName);
    }

    return Output;
}

std::vector<std::string> RenderCore::GetAvailablePhysicalDeviceLayerExtensionsNames(std::string_view const LayerName)
{
    std::vector<std::string> Output;
    for (VkExtensionProperties const& ExtensionIter: GetAvailablePhysicalDeviceLayerExtensions(LayerName))
    {
        Output.emplace_back(ExtensionIter.extensionName);
    }

    return Output;
}

std::vector<std::string> RenderCore::GetAvailablePhysicalDeviceLayersNames()
{
    std::vector<std::string> Output;
    for (VkLayerProperties const& LayerIter: GetAvailablePhysicalDeviceLayers())
    {
        Output.emplace_back(LayerIter.layerName);
    }

    return Output;
}

VkSurfaceCapabilitiesKHR RenderCore::GetAvailablePhysicalDeviceSurfaceCapabilities()
{
    if (g_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    VkSurfaceCapabilitiesKHR Output;
    CheckVulkanResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_PhysicalDevice, GetSurface(), &Output));

    return Output;
}

std::vector<VkSurfaceFormatKHR> RenderCore::GetAvailablePhysicalDeviceSurfaceFormats()
{
    if (g_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    VkSurfaceKHR const& VulkanSurface = GetSurface();

    std::uint32_t Count = 0U;
    CheckVulkanResult(vkGetPhysicalDeviceSurfaceFormatsKHR(g_PhysicalDevice, VulkanSurface, &Count, nullptr));

    std::vector Output(Count, VkSurfaceFormatKHR());
    CheckVulkanResult(vkGetPhysicalDeviceSurfaceFormatsKHR(g_PhysicalDevice, VulkanSurface, &Count, Output.data()));

    return Output;
}

std::vector<VkPresentModeKHR> RenderCore::GetAvailablePhysicalDeviceSurfacePresentationModes()
{
    if (g_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    VkSurfaceKHR const& VulkanSurface = GetSurface();

    std::uint32_t Count = 0U;
    CheckVulkanResult(vkGetPhysicalDeviceSurfacePresentModesKHR(g_PhysicalDevice, VulkanSurface, &Count, nullptr));

    std::vector Output(Count, VkPresentModeKHR());
    CheckVulkanResult(vkGetPhysicalDeviceSurfacePresentModesKHR(g_PhysicalDevice, VulkanSurface, &Count, Output.data()));

    return Output;
}

VkDeviceSize RenderCore::GetMinUniformBufferOffsetAlignment()
{
    if (g_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    VkPhysicalDeviceProperties DeviceProperties;
    vkGetPhysicalDeviceProperties(g_PhysicalDevice, &DeviceProperties);

    return DeviceProperties.limits.minUniformBufferOffsetAlignment;
}