// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <GLFW/glfw3.h>
#include <boost/log/trivial.hpp>
#include <volk.h>

module RenderCoreDeviceManager;

import <optional>;
import <string_view>;
import <vector>;
import <cstdint>;
import <unordered_map>;

import RenderCoreEnumHelpers;
import RenderCoreDeviceProperties;

using namespace RenderCore;

DeviceManager::DeviceManager()
    : m_PhysicalDevice(VK_NULL_HANDLE),
      m_Device(VK_NULL_HANDLE)
{
}

DeviceManager::~DeviceManager()
{
    try
    {
        Shutdown();
    }
    catch (...)
    {
    }
}

DeviceManager& DeviceManager::Get()
{
    static DeviceManager Instance {};
    return Instance;
}

void DeviceManager::PickPhysicalDevice()
{
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

void DeviceManager::CreateLogicalDevice()
{
    std::optional<std::uint8_t> GraphicsQueueFamilyIndex     = std::nullopt;
    std::optional<std::uint8_t> PresentationQueueFamilyIndex = std::nullopt;
    std::optional<std::uint8_t> TransferQueueFamilyIndex     = std::nullopt;

    if (!GetQueueFamilyIndices(GraphicsQueueFamilyIndex, PresentationQueueFamilyIndex, TransferQueueFamilyIndex))
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

    std::vector Layers(g_RequiredDeviceLayers.begin(), g_RequiredDeviceLayers.end());
    std::vector Extensions(g_RequiredDeviceExtensions.begin(), g_RequiredDeviceExtensions.end());

#ifdef _DEBUG
    Layers.insert(Layers.end(), g_DebugDeviceLayers.begin(), g_DebugDeviceLayers.end());
    Extensions.insert(Extensions.end(), g_DebugDeviceExtensions.begin(), g_DebugDeviceExtensions.end());
#endif

    std::unordered_map<std::uint8_t, std::uint8_t> QueueFamilyIndices;
    QueueFamilyIndices.emplace(m_GraphicsQueue.first, 1U);
    if (!QueueFamilyIndices.contains(m_PresentationQueue.first))
    {
        QueueFamilyIndices.emplace(m_PresentationQueue.first, 1U);
    }
    else
    {
        ++QueueFamilyIndices[m_PresentationQueue.first];
    }

    if (!QueueFamilyIndices.contains(m_TransferQueue.first))
    {
        QueueFamilyIndices.emplace(m_TransferQueue.first, 1U);
    }
    else
    {
        ++QueueFamilyIndices[m_TransferQueue.first];
    }

    m_UniqueQueueFamilyIndices.clear();
    std::unordered_map<std::uint32_t, std::vector<float>> QueuePriorities;
    for (auto const& [Index, Quantity]: QueueFamilyIndices)
    {
        m_UniqueQueueFamilyIndices.push_back(Index);
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

    constexpr VkPhysicalDeviceFeatures DeviceFeatures {
            .samplerAnisotropy = VK_TRUE};

    VkDeviceCreateInfo const DeviceCreateInfo {
            .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount    = static_cast<std::uint32_t>(QueueCreateInfo.size()),
            .pQueueCreateInfos       = QueueCreateInfo.data(),
            .enabledLayerCount       = static_cast<std::uint32_t>(Layers.size()),
            .ppEnabledLayerNames     = Layers.data(),
            .enabledExtensionCount   = static_cast<std::uint32_t>(Extensions.size()),
            .ppEnabledExtensionNames = Extensions.data(),
            .pEnabledFeatures        = &DeviceFeatures};

    Helpers::CheckVulkanResult(vkCreateDevice(m_PhysicalDevice, &DeviceCreateInfo, nullptr, &m_Device));

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

bool DeviceManager::UpdateDeviceProperties(GLFWwindow* const Window) const
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
        GetDeviceProperties().Extent = Helpers::GetWindowExtent(Window, GetDeviceProperties().Capabilities);
    }

    GetDeviceProperties().Format = SupportedFormats[0];
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
        vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, FormatIter, &FormatProperties);

        if ((FormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0U)
        {
            GetDeviceProperties().DepthFormat = FormatIter;
            break;
        }
    }

    return GetDeviceProperties().IsValid();
}

VulkanDeviceProperties& DeviceManager::GetDeviceProperties()
{
    static VulkanDeviceProperties s_DeviceProperties {};
    return s_DeviceProperties;
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

std::vector<std::uint8_t>& DeviceManager::GetUniqueQueueFamilyIndices()
{
    return m_UniqueQueueFamilyIndices;
}

std::vector<std::uint32_t> DeviceManager::GetUniqueQueueFamilyIndicesU32()
{
    std::vector<std::uint32_t> QueueFamilyIndicesU32(m_UniqueQueueFamilyIndices.size());
    std::ranges::transform(
            m_UniqueQueueFamilyIndices,
            QueueFamilyIndicesU32.begin(),
            [](std::uint8_t const& Index) {
                return static_cast<std::uint32_t>(Index);
            });
    return QueueFamilyIndicesU32;
}

std::uint32_t DeviceManager::GetMinImageCount() const
{
    bool const SupportsTripleBuffering = GetDeviceProperties().Capabilities.minImageCount < 3U && GetDeviceProperties().Capabilities.maxImageCount >= 3U;
    return SupportsTripleBuffering ? 3U : GetDeviceProperties().Capabilities.minImageCount;
}

void DeviceManager::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan device manager";

    vkDestroyDevice(m_Device, nullptr);
    m_Device = VK_NULL_HANDLE;

    m_PhysicalDevice           = VK_NULL_HANDLE;
    m_GraphicsQueue.second     = VK_NULL_HANDLE;
    m_PresentationQueue.second = VK_NULL_HANDLE;
    m_TransferQueue.second     = VK_NULL_HANDLE;
}

bool DeviceManager::IsInitialized() const
{
    return m_PhysicalDevice != VK_NULL_HANDLE
           && m_Device != VK_NULL_HANDLE
           && m_GraphicsQueue.second != VK_NULL_HANDLE
           && m_PresentationQueue.second != VK_NULL_HANDLE
           && m_TransferQueue.second != VK_NULL_HANDLE;
}

std::vector<VkPhysicalDevice> DeviceManager::GetAvailablePhysicalDevices()
{
    VkInstance const& VulkanInstance = EngineCore::Get().GetInstance();

    std::uint32_t DeviceCount = 0U;
    Helpers::CheckVulkanResult(vkEnumeratePhysicalDevices(VulkanInstance, &DeviceCount, nullptr));

    std::vector<VkPhysicalDevice> Output(DeviceCount, VK_NULL_HANDLE);
    Helpers::CheckVulkanResult(vkEnumeratePhysicalDevices(VulkanInstance, &DeviceCount, Output.data()));

    return Output;
}

std::vector<VkExtensionProperties> DeviceManager::GetAvailablePhysicalDeviceExtensions() const
{
    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    std::uint32_t ExtensionsCount = 0;
    Helpers::CheckVulkanResult(vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &ExtensionsCount, nullptr));

    std::vector<VkExtensionProperties> Output(ExtensionsCount);
    Helpers::CheckVulkanResult(vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &ExtensionsCount, Output.data()));

    return Output;
}

std::vector<VkLayerProperties> DeviceManager::GetAvailablePhysicalDeviceLayers() const
{
    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    std::uint32_t LayersCount = 0;
    Helpers::CheckVulkanResult(vkEnumerateDeviceLayerProperties(m_PhysicalDevice, &LayersCount, nullptr));

    std::vector<VkLayerProperties> Output(LayersCount);
    Helpers::CheckVulkanResult(vkEnumerateDeviceLayerProperties(m_PhysicalDevice, &LayersCount, Output.data()));

    return Output;
}

std::vector<VkExtensionProperties> DeviceManager::GetAvailablePhysicalDeviceLayerExtensions(std::string_view const LayerName) const
{
    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    if (std::vector<std::string> const AvailableLayers = GetAvailablePhysicalDeviceLayersNames();
        std::ranges::find(AvailableLayers, LayerName) == AvailableLayers.end())
    {
        return {};
    }

    std::uint32_t ExtensionsCount = 0;
    Helpers::CheckVulkanResult(vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, LayerName.data(), &ExtensionsCount, nullptr));

    std::vector<VkExtensionProperties> Output(ExtensionsCount);
    Helpers::CheckVulkanResult(vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, LayerName.data(), &ExtensionsCount, Output.data()));

    return Output;
}

std::vector<std::string> DeviceManager::GetAvailablePhysicalDeviceExtensionsNames() const
{
    std::vector<std::string> Output;
    for (VkExtensionProperties const& ExtensionIter: GetAvailablePhysicalDeviceExtensions())
    {
        Output.emplace_back(ExtensionIter.extensionName);
    }

    return Output;
}

std::vector<std::string> DeviceManager::GetAvailablePhysicalDeviceLayerExtensionsNames(std::string_view const LayerName) const
{
    std::vector<std::string> Output;
    for (VkExtensionProperties const& ExtensionIter: GetAvailablePhysicalDeviceLayerExtensions(LayerName))
    {
        Output.emplace_back(ExtensionIter.extensionName);
    }

    return Output;
}

std::vector<std::string> DeviceManager::GetAvailablePhysicalDeviceLayersNames() const
{
    std::vector<std::string> Output;
    for (VkLayerProperties const& LayerIter: GetAvailablePhysicalDeviceLayers())
    {
        Output.emplace_back(LayerIter.layerName);
    }

    return Output;
}

VkSurfaceCapabilitiesKHR DeviceManager::GetAvailablePhysicalDeviceSurfaceCapabilities() const
{
    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    VkSurfaceCapabilitiesKHR Output;
    Helpers::CheckVulkanResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, EngineCore::Get().GetSurface(), &Output));

    return Output;
}

std::vector<VkSurfaceFormatKHR> DeviceManager::GetAvailablePhysicalDeviceSurfaceFormats() const
{
    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    VkSurfaceKHR const& VulkanSurface = EngineCore::Get().GetSurface();

    std::uint32_t Count = 0U;
    Helpers::CheckVulkanResult(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, VulkanSurface, &Count, nullptr));

    std::vector Output(Count, VkSurfaceFormatKHR());
    Helpers::CheckVulkanResult(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, VulkanSurface, &Count, Output.data()));

    return Output;
}

std::vector<VkPresentModeKHR> DeviceManager::GetAvailablePhysicalDeviceSurfacePresentationModes() const
{
    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    VkSurfaceKHR const& VulkanSurface = EngineCore::Get().GetSurface();

    std::uint32_t Count = 0U;
    Helpers::CheckVulkanResult(vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, VulkanSurface, &Count, nullptr));

    std::vector Output(Count, VkPresentModeKHR());
    Helpers::CheckVulkanResult(vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, VulkanSurface, &Count, Output.data()));

    return Output;
}

VkDeviceSize DeviceManager::GetMinUniformBufferOffsetAlignment() const
{
    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    VkPhysicalDeviceProperties DeviceProperties;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &DeviceProperties);

    return DeviceProperties.limits.minUniformBufferOffsetAlignment;
}

bool DeviceManager::IsPhysicalDeviceSuitable(VkPhysicalDevice const& Device)
{
    if (Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    VkPhysicalDeviceProperties DeviceProperties;
    vkGetPhysicalDeviceProperties(Device, &DeviceProperties);

    VkPhysicalDeviceFeatures SupportedFeatures;
    vkGetPhysicalDeviceFeatures(Device, &SupportedFeatures);

    return DeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && SupportedFeatures.samplerAnisotropy != 0U;
}

bool DeviceManager::GetQueueFamilyIndices(std::optional<std::uint8_t>& GraphicsQueueFamilyIndex,
                                          std::optional<std::uint8_t>& PresentationQueueFamilyIndex,
                                          std::optional<std::uint8_t>& TransferQueueFamilyIndex) const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Getting queue family indices";

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    VkSurfaceKHR const& VulkanSurface = EngineCore::Get().GetSurface();

    std::uint32_t QueueFamilyCount = 0U;
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &QueueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &QueueFamilyCount, QueueFamilies.data());

    for (std::uint32_t Iterator = 0U; Iterator < QueueFamilyCount; ++Iterator)
    {
        if (!GraphicsQueueFamilyIndex.has_value() && (QueueFamilies[Iterator].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U)
        {
            GraphicsQueueFamilyIndex = static_cast<std::uint8_t>(Iterator);
        }
        else if (!TransferQueueFamilyIndex.has_value() && (QueueFamilies[Iterator].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0U)
        {
            TransferQueueFamilyIndex = static_cast<std::uint8_t>(Iterator);
        }
        else if (!PresentationQueueFamilyIndex.has_value())
        {
            VkBool32 PresentationSupport = 0U;
            Helpers::CheckVulkanResult(vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, Iterator, VulkanSurface, &PresentationSupport));

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

void DeviceManager::ListAvailablePhysicalDevices()
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

void DeviceManager::ListAvailablePhysicalDeviceExtensions() const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available vulkan physical device extensions...";

    for (auto const& [ExtName, SpecVer]: GetAvailablePhysicalDeviceExtensions())
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Name: " << ExtName;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Spec Version: " << SpecVer << std::endl;
    }
}

void DeviceManager::ListAvailablePhysicalDeviceLayers() const
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

void DeviceManager::ListAvailablePhysicalDeviceLayerExtensions(std::string_view const LayerName) const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available vulkan physical device layer '" << LayerName << "' extensions...";

    for (auto const& [ExtName, SpecVer]: GetAvailablePhysicalDeviceLayerExtensions(LayerName))
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Name: " << ExtName;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Spec Version: " << SpecVer << std::endl;
    }
}

void DeviceManager::ListAvailablePhysicalDeviceSurfaceCapabilities() const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available vulkan physical device surface capabilities...";

    VkSurfaceCapabilitiesKHR const SurfaceCapabilities = GetAvailablePhysicalDeviceSurfaceCapabilities();

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Min Image Count: " << SurfaceCapabilities.minImageCount;
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Max Image Count: " << SurfaceCapabilities.maxImageCount;
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Current Extent: (" << SurfaceCapabilities.currentExtent.width << ", " << SurfaceCapabilities.currentExtent.height << ")";
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Min Image Extent: (" << SurfaceCapabilities.minImageExtent.width << ", " << SurfaceCapabilities.minImageExtent.height << ")";
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Max Image Extent: (" << SurfaceCapabilities.maxImageExtent.width << ", " << SurfaceCapabilities.maxImageExtent.height << ")";
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Max Image Array Layers: " << SurfaceCapabilities.maxImageArrayLayers;
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Supported Transforms: " << TransformFlagToString(static_cast<VkSurfaceTransformFlagBitsKHR>(SurfaceCapabilities.supportedTransforms));
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Current Transform: " << TransformFlagToString(SurfaceCapabilities.currentTransform);
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Supported Composite Alpha: " << CompositeAlphaFlagToString(SurfaceCapabilities.supportedCompositeAlpha);
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Supported Usage Flags: " << ImageUsageFlagToString(static_cast<VkImageUsageFlagBits>(SurfaceCapabilities.supportedUsageFlags));
}

void DeviceManager::ListAvailablePhysicalDeviceSurfaceFormats() const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available vulkan physical device surface formats...";

    for (auto const& [Format, ColorSpace]: GetAvailablePhysicalDeviceSurfaceFormats())
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Format: " << SurfaceFormatToString(Format);
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Color Space: " << ColorSpaceModeToString(ColorSpace) << std::endl;
    }
}

void DeviceManager::ListAvailablePhysicalDeviceSurfacePresentationModes() const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available vulkan physical device presentation modes...";

    for (VkPresentModeKHR const& FormatIter: GetAvailablePhysicalDeviceSurfacePresentationModes())
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Mode: " << PresentationModeToString(FormatIter);
    }
}

#endif