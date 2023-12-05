// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <boost/log/trivial.hpp>
#include <volk.h>

module RenderCore.Management.DeviceManagement;

import <optional>;

import RenderCore.Management.BufferManagement;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import RenderCore.Utils.EnumConverter;
import Timer.ExecutionCounter;

using namespace RenderCore;

bool IsPhysicalDeviceSuitable(VkPhysicalDevice const& Device)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

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

bool DeviceManager::GetQueueFamilyIndices(VkSurfaceKHR const& VulkanSurface,
                           std::optional<std::uint8_t>& GraphicsQueueFamilyIndex,
                           std::optional<std::uint8_t>& PresentationQueueFamilyIndex,
                           std::optional<std::uint8_t>& TransferQueueFamilyIndex) const
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Getting queue family indices";

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    std::uint32_t QueueFamilyCount = 0U;
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &QueueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &QueueFamilyCount, std::data(QueueFamilies));

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
            CheckVulkanResult(vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, Iterator, VulkanSurface, &PresentationSupport));

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

void DeviceManager::PickPhysicalDevice()
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Picking a physical device";

    for (VkPhysicalDevice const& DeviceIter: GetAvailablePhysicalDevices())
    {
        if (m_PhysicalDevice == VK_NULL_HANDLE && IsPhysicalDeviceSuitable(DeviceIter))
        {
            m_PhysicalDevice = DeviceIter;
            break;
        }
    }

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("No suitable Vulkan physical device found.");
    }
}

void DeviceManager::CreateLogicalDevice(VkSurfaceKHR const& VulkanSurface)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    std::optional<std::uint8_t> GraphicsQueueFamilyIndex {std::nullopt};
    std::optional<std::uint8_t> PresentationQueueFamilyIndex {std::nullopt};
    std::optional<std::uint8_t> TransferQueueFamilyIndex {std::nullopt};

    if (!GetQueueFamilyIndices(VulkanSurface, GraphicsQueueFamilyIndex, PresentationQueueFamilyIndex, TransferQueueFamilyIndex))
    {
        throw std::runtime_error("Failed to get queue family indices.");
    }

    m_GraphicsQueue.first     = GraphicsQueueFamilyIndex.value();
    m_PresentationQueue.first = PresentationQueueFamilyIndex.value();
    m_TransferQueue.first     = TransferQueueFamilyIndex.value();

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan logical device";

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    std::vector Layers(std::cbegin(g_RequiredDeviceLayers), std::cend(g_RequiredDeviceLayers));
    std::vector Extensions(std::cbegin(g_RequiredDeviceExtensions), std::cend(g_RequiredDeviceExtensions));

#ifdef _DEBUG
    Layers.insert(std::cend(Layers), std::cbegin(g_DebugDeviceLayers), std::cend(g_DebugDeviceLayers));
    Extensions.insert(std::cend(Extensions), std::cbegin(g_DebugDeviceExtensions), std::cend(g_DebugDeviceExtensions));
#endif

    std::unordered_map<std::uint8_t, std::uint8_t> QueueFamilyIndices {{m_GraphicsQueue.first, 1U}};
    if (!QueueFamilyIndices.contains(m_PresentationQueue.first))
    {
        QueueFamilyIndices.emplace(m_PresentationQueue.first, 1U);
    }
    else
    {
        ++QueueFamilyIndices.at(m_PresentationQueue.first);
    }

    if (!QueueFamilyIndices.contains(m_TransferQueue.first))
    {
        QueueFamilyIndices.emplace(m_TransferQueue.first, 1U);
    }
    else
    {
        ++QueueFamilyIndices.at(m_TransferQueue.first);
    }

    m_UniqueQueueFamilyIndices.clear();
    m_UniqueQueueFamilyIndices.reserve(std::size(QueueFamilyIndices));

    std::vector<VkDeviceQueueCreateInfo> QueueCreateInfo;
    QueueCreateInfo.reserve(std::size(QueueFamilyIndices));
    for (auto const& [Index, Count]: QueueFamilyIndices)
    {
        m_UniqueQueueFamilyIndices.push_back(Index);

        QueueCreateInfo.push_back(
                VkDeviceQueueCreateInfo {
                        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                        .queueFamilyIndex = Index,
                        .queueCount       = Count,
                        .pQueuePriorities = std::data(std::vector(Count, 1.0F))});
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

    CheckVulkanResult(vkCreateDevice(m_PhysicalDevice, &DeviceCreateInfo, nullptr, &m_Device));

    if (vkGetDeviceQueue(m_Device, m_GraphicsQueue.first, 0U, &m_GraphicsQueue.second);
        m_GraphicsQueue.second == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to get graphics queue.");
    }

    if (vkGetDeviceQueue(m_Device, m_PresentationQueue.first, 0U, &m_PresentationQueue.second);
        m_PresentationQueue.second == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to get presentation queue.");
    }

    if (vkGetDeviceQueue(m_Device, m_TransferQueue.first, 0U, &m_TransferQueue.second);
        m_TransferQueue.second == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to get transfer queue.");
    }
}

VkSurfaceCapabilitiesKHR DeviceManager::GetSurfaceCapabilities(VkSurfaceKHR const& VulkanSurface) const
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    VkSurfaceCapabilitiesKHR Output;
    CheckVulkanResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, VulkanSurface, &Output));

    return Output;
}

SurfaceProperties DeviceManager::GetSurfaceProperties(GLFWwindow* const Window, VkSurfaceKHR const& VulkanSurface) const
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    std::vector<VkSurfaceFormatKHR> const SupportedFormats = GetAvailablePhysicalDeviceSurfaceFormats(m_PhysicalDevice, VulkanSurface);
    if (std::empty(SupportedFormats))
    {
        throw std::runtime_error("No supported surface formats found.");
    }

    std::vector<VkPresentModeKHR> const SupportedPresentationModes = GetAvailablePhysicalDeviceSurfacePresentationModes(m_PhysicalDevice, VulkanSurface);
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
        vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, FormatIter, &FormatProperties);

        if ((FormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0U)
        {
            Output.DepthFormat = FormatIter;
            break;
        }
    }

    return Output;
}

VkDevice& DeviceManager::GetLogicalDevice()
{
    return m_Device;
}

VkPhysicalDevice& DeviceManager::GetPhysicalDevice()
{
    return m_PhysicalDevice;
}

std::pair<std::uint8_t, VkQueue>& DeviceManager::GetGraphicsQueue()
{
    return m_GraphicsQueue;
}

std::pair<std::uint8_t, VkQueue>& DeviceManager::GetPresentationQueue()
{
    return m_PresentationQueue;
}

std::pair<std::uint8_t, VkQueue>& DeviceManager::GetTransferQueue()
{
    return m_TransferQueue;
}

std::vector<std::uint32_t> DeviceManager::GetUniqueQueueFamilyIndicesU32() const
{
    std::vector<std::uint32_t> QueueFamilyIndicesU32(std::size(m_UniqueQueueFamilyIndices));

    std::ranges::transform(
            m_UniqueQueueFamilyIndices,
            std::begin(QueueFamilyIndicesU32),
            [](std::uint8_t const& Index) {
                return static_cast<std::uint32_t>(Index);
            });

    return QueueFamilyIndicesU32;
}

void DeviceManager::ReleaseDeviceResources()
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Releasing vulkan device resources";

    vkDestroyDevice(m_Device, nullptr);
    m_Device = VK_NULL_HANDLE;

    m_PhysicalDevice           = VK_NULL_HANDLE;
    m_GraphicsQueue.second     = VK_NULL_HANDLE;
    m_PresentationQueue.second = VK_NULL_HANDLE;
    m_TransferQueue.second     = VK_NULL_HANDLE;
}

std::vector<VkPhysicalDevice> RenderCore::GetAvailablePhysicalDevices()
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    VkInstance const& VulkanInstance = volkGetLoadedInstance();

    std::uint32_t DeviceCount = 0U;
    CheckVulkanResult(vkEnumeratePhysicalDevices(VulkanInstance, &DeviceCount, nullptr));

    std::vector<VkPhysicalDevice> Output(DeviceCount, VK_NULL_HANDLE);
    CheckVulkanResult(vkEnumeratePhysicalDevices(VulkanInstance, &DeviceCount, std::data(Output)));

    return Output;
}

std::vector<VkExtensionProperties> RenderCore::GetAvailablePhysicalDeviceExtensions(VkPhysicalDevice const& PhysicalDevice)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

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
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

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
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

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
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    std::vector<std::string> Output;
    for (VkExtensionProperties const& ExtensionIter: GetAvailablePhysicalDeviceExtensions(PhysicalDevice))
    {
        Output.emplace_back(ExtensionIter.extensionName);
    }

    return Output;
}

std::vector<std::string> RenderCore::GetAvailablePhysicalDeviceLayerExtensionsNames(VkPhysicalDevice const& PhysicalDevice, std::string_view const& LayerName)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    std::vector<std::string> Output;
    for (VkExtensionProperties const& ExtensionIter: GetAvailablePhysicalDeviceLayerExtensions(PhysicalDevice, LayerName))
    {
        Output.emplace_back(ExtensionIter.extensionName);
    }

    return Output;
}

std::vector<std::string> RenderCore::GetAvailablePhysicalDeviceLayersNames(VkPhysicalDevice const& PhysicalDevice)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    std::vector<std::string> Output;
    for (VkLayerProperties const& LayerIter: GetAvailablePhysicalDeviceLayers(PhysicalDevice))
    {
        Output.emplace_back(LayerIter.layerName);
    }

    return Output;
}

std::vector<VkSurfaceFormatKHR> RenderCore::GetAvailablePhysicalDeviceSurfaceFormats(VkPhysicalDevice const& PhysicalDevice, VkSurfaceKHR const& VulkanSurface)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

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
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

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

VkDeviceSize DeviceManager::GetMinUniformBufferOffsetAlignment() const
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    VkPhysicalDeviceProperties SurfaceProperties;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &SurfaceProperties);

    return SurfaceProperties.limits.minUniformBufferOffsetAlignment;
}