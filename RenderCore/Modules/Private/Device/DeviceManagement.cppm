// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <boost/log/trivial.hpp>
#include <optional>
#include <volk.h>

module RenderCore.Management.DeviceManagement;

import RenderCore.Management.BufferManagement;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import RenderCore.Utils.EnumConverter;
import Timer.ExecutionCounter;

using namespace RenderCore;

VkPhysicalDevice g_PhysicalDevice {VK_NULL_HANDLE};
VkDevice g_Device {VK_NULL_HANDLE};
std::pair<std::uint8_t, VkQueue> g_GraphicsQueue {};
std::pair<std::uint8_t, VkQueue> g_PresentationQueue {};
std::pair<std::uint8_t, VkQueue> g_TransferQueue {};
std::vector<std::uint8_t> g_UniqueQueueFamilyIndices {};

bool IsPhysicalDeviceSuitable(VkPhysicalDevice const& Device)
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

bool GetQueueFamilyIndices(VkSurfaceKHR const& VulkanSurface,
                           std::optional<std::uint8_t>& GraphicsQueueFamilyIndex,
                           std::optional<std::uint8_t>& PresentationQueueFamilyIndex,
                           std::optional<std::uint8_t>& TransferQueueFamilyIndex)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Getting queue family indices";

    if (g_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    std::uint32_t QueueFamilyCount = 0U;
    vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &QueueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &QueueFamilyCount, std::data(QueueFamilies));

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

void PickPhysicalDevice()
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

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
}

void CreateLogicalDevice(VkSurfaceKHR const& VulkanSurface)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    std::optional<std::uint8_t> GraphicsQueueFamilyIndex {std::nullopt};
    std::optional<std::uint8_t> PresentationQueueFamilyIndex {std::nullopt};
    std::optional<std::uint8_t> TransferQueueFamilyIndex {std::nullopt};

    if (!GetQueueFamilyIndices(VulkanSurface, GraphicsQueueFamilyIndex, PresentationQueueFamilyIndex, TransferQueueFamilyIndex))
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

    std::vector Layers(std::cbegin(g_RequiredDeviceLayers), std::cend(g_RequiredDeviceLayers));
    std::vector Extensions(std::cbegin(g_RequiredDeviceExtensions), std::cend(g_RequiredDeviceExtensions));

#ifdef _DEBUG
    Layers.insert(std::cend(Layers), std::cbegin(g_DebugDeviceLayers), std::cend(g_DebugDeviceLayers));
    Extensions.insert(std::cend(Extensions), std::cbegin(g_DebugDeviceExtensions), std::cend(g_DebugDeviceExtensions));
#endif

    std::unordered_map<std::uint8_t, std::uint8_t> QueueFamilyIndices {{g_GraphicsQueue.first, 1U}};
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
    g_UniqueQueueFamilyIndices.reserve(std::size(QueueFamilyIndices));

    std::vector<VkDeviceQueueCreateInfo> QueueCreateInfo;
    QueueCreateInfo.reserve(std::size(QueueFamilyIndices));

    std::vector<std::vector<float>> PriorityHandles;
    PriorityHandles.reserve(std::size(QueueFamilyIndices));

    for (auto const& [Index, Count]: QueueFamilyIndices)
    {
        g_UniqueQueueFamilyIndices.push_back(Index);
        PriorityHandles.emplace_back(Count, 1.F);

        QueueCreateInfo.push_back(
                VkDeviceQueueCreateInfo {
                        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                        .queueFamilyIndex = Index,
                        .queueCount       = Count,
                        .pQueuePriorities = std::data(PriorityHandles.back())});
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
            .queueCreateInfoCount    = static_cast<std::uint32_t>(std::size(QueueCreateInfo)),
            .pQueueCreateInfos       = std::data(QueueCreateInfo),
            .enabledLayerCount       = static_cast<std::uint32_t>(std::size(Layers)),
            .ppEnabledLayerNames     = std::data(Layers),
            .enabledExtensionCount   = static_cast<std::uint32_t>(std::size(Extensions)),
            .ppEnabledExtensionNames = std::data(Extensions),
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

void RenderCore::InitializeDevice(VkSurfaceKHR const& VulkanSurface)
{
    if (g_PhysicalDevice != VK_NULL_HANDLE)
    {
        return;
    }

    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Initializing vulkan devices";

    PickPhysicalDevice();
    CreateLogicalDevice(VulkanSurface);
}

VkSurfaceCapabilitiesKHR RenderCore::GetSurfaceCapabilities(VkSurfaceKHR const& VulkanSurface)
{
    if (g_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    VkSurfaceCapabilitiesKHR Output;
    CheckVulkanResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_PhysicalDevice, VulkanSurface, &Output));

    return Output;
}

SurfaceProperties RenderCore::GetSurfaceProperties(GLFWwindow* const Window, VkSurfaceKHR const& VulkanSurface)
{
    std::vector<VkSurfaceFormatKHR> const SupportedFormats = GetAvailablePhysicalDeviceSurfaceFormats(g_PhysicalDevice, VulkanSurface);
    if (std::empty(SupportedFormats))
    {
        throw std::runtime_error("No supported surface formats found.");
    }

    std::vector<VkPresentModeKHR> const SupportedPresentationModes = GetAvailablePhysicalDeviceSurfacePresentationModes(g_PhysicalDevice, VulkanSurface);
    if (std::empty(SupportedFormats))
    {
        throw std::runtime_error("No supported presentation modes found.");
    }

    SurfaceProperties Output {};
    Output.Extent = GetWindowExtent(Window, GetSurfaceCapabilities(VulkanSurface));

    Output.Format = SupportedFormats.front();
    if (auto const MatchingFormat = std::ranges::find_if(
                SupportedFormats,
                [](VkSurfaceFormatKHR const& Iter) {
                    return Iter.format == VK_FORMAT_B8G8R8A8_SRGB && Iter.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                });
        MatchingFormat != std::cend(SupportedFormats))
    {
        Output.Format = *MatchingFormat;
    }

    Output.Mode = VK_PRESENT_MODE_FIFO_KHR;
    if (auto const MatchingMode = std::ranges::find_if(
                SupportedPresentationModes,
                [](VkPresentModeKHR const& Iter) {
                    return Iter == VK_PRESENT_MODE_MAILBOX_KHR;
                });
        MatchingMode != std::cend(SupportedPresentationModes))
    {
        Output.Mode = *MatchingMode;
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
            Output.DepthFormat = FormatIter;
            break;
        }
    }

    return Output;
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

std::vector<std::uint32_t> RenderCore::GetUniqueQueueFamilyIndicesU32()
{
    std::vector<std::uint32_t> QueueFamilyIndicesU32(std::size(g_UniqueQueueFamilyIndices));

    std::ranges::transform(
            g_UniqueQueueFamilyIndices,
            std::begin(QueueFamilyIndicesU32),
            [](std::uint8_t const& Index) {
                return static_cast<std::uint32_t>(Index);
            });

    return QueueFamilyIndicesU32;
}

void RenderCore::ReleaseDeviceResources()
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

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
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    VkInstance const& VulkanInstance = volkGetLoadedInstance();

    std::uint32_t DeviceCount = 0U;
    CheckVulkanResult(vkEnumeratePhysicalDevices(VulkanInstance, &DeviceCount, nullptr));

    std::vector<VkPhysicalDevice> Output(DeviceCount, VK_NULL_HANDLE);
    CheckVulkanResult(vkEnumeratePhysicalDevices(VulkanInstance, &DeviceCount, std::data(Output)));

    return Output;
}

std::vector<VkExtensionProperties> RenderCore::GetAvailablePhysicalDeviceExtensions(VkPhysicalDevice const& PhysicalDevice)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    if (PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    std::uint32_t ExtensionsCount = 0;
    CheckVulkanResult(vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &ExtensionsCount, nullptr));

    std::vector<VkExtensionProperties> Output(ExtensionsCount);
    CheckVulkanResult(vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &ExtensionsCount, std::data(Output)));

    return Output;
}

std::vector<VkLayerProperties> RenderCore::GetAvailablePhysicalDeviceLayers(VkPhysicalDevice const& PhysicalDevice)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    if (PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    std::uint32_t LayersCount = 0;
    CheckVulkanResult(vkEnumerateDeviceLayerProperties(PhysicalDevice, &LayersCount, nullptr));

    std::vector<VkLayerProperties> Output(LayersCount);
    CheckVulkanResult(vkEnumerateDeviceLayerProperties(PhysicalDevice, &LayersCount, std::data(Output)));

    return Output;
}

std::vector<VkExtensionProperties> RenderCore::GetAvailablePhysicalDeviceLayerExtensions(VkPhysicalDevice const& PhysicalDevice, std::string_view const& LayerName)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    if (PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    if (std::vector<std::string> const AvailableLayers = GetAvailablePhysicalDeviceLayersNames(PhysicalDevice);
        std::ranges::find(AvailableLayers, LayerName) == std::cend(AvailableLayers))
    {
        return {};
    }

    std::uint32_t ExtensionsCount = 0;
    CheckVulkanResult(vkEnumerateDeviceExtensionProperties(PhysicalDevice, std::data(LayerName), &ExtensionsCount, nullptr));

    std::vector<VkExtensionProperties> Output(ExtensionsCount);
    CheckVulkanResult(vkEnumerateDeviceExtensionProperties(PhysicalDevice, std::data(LayerName), &ExtensionsCount, std::data(Output)));

    return Output;
}

std::vector<std::string> RenderCore::GetAvailablePhysicalDeviceExtensionsNames(VkPhysicalDevice const& PhysicalDevice)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    std::vector<std::string> Output;
    for (VkExtensionProperties const& ExtensionIter: GetAvailablePhysicalDeviceExtensions(PhysicalDevice))
    {
        Output.emplace_back(ExtensionIter.extensionName);
    }

    return Output;
}

std::vector<std::string> RenderCore::GetAvailablePhysicalDeviceLayerExtensionsNames(VkPhysicalDevice const& PhysicalDevice, std::string_view const& LayerName)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    std::vector<std::string> Output;
    for (VkExtensionProperties const& ExtensionIter: GetAvailablePhysicalDeviceLayerExtensions(PhysicalDevice, LayerName))
    {
        Output.emplace_back(ExtensionIter.extensionName);
    }

    return Output;
}

std::vector<std::string> RenderCore::GetAvailablePhysicalDeviceLayersNames(VkPhysicalDevice const& PhysicalDevice)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    std::vector<std::string> Output;
    for (VkLayerProperties const& LayerIter: GetAvailablePhysicalDeviceLayers(PhysicalDevice))
    {
        Output.emplace_back(LayerIter.layerName);
    }

    return Output;
}

std::vector<VkSurfaceFormatKHR> RenderCore::GetAvailablePhysicalDeviceSurfaceFormats(VkPhysicalDevice const& PhysicalDevice, VkSurfaceKHR const& VulkanSurface)
{
    if (PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    std::uint32_t Count = 0U;
    CheckVulkanResult(vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, VulkanSurface, &Count, nullptr));

    std::vector Output(Count, VkSurfaceFormatKHR());
    CheckVulkanResult(vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, VulkanSurface, &Count, std::data(Output)));

    return Output;
}

std::vector<VkPresentModeKHR> RenderCore::GetAvailablePhysicalDeviceSurfacePresentationModes(VkPhysicalDevice const& PhysicalDevice, VkSurfaceKHR const& VulkanSurface)
{
    if (PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    std::uint32_t Count = 0U;
    CheckVulkanResult(vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, VulkanSurface, &Count, nullptr));

    std::vector Output(Count, VkPresentModeKHR());
    CheckVulkanResult(vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, VulkanSurface, &Count, std::data(Output)));

    return Output;
}

VkDeviceSize RenderCore::GetMinUniformBufferOffsetAlignment()
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    if (g_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    VkPhysicalDeviceProperties SurfaceProperties;
    vkGetPhysicalDeviceProperties(g_PhysicalDevice, &SurfaceProperties);

    return SurfaceProperties.limits.minUniformBufferOffsetAlignment;
}