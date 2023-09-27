// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "VulkanRenderCore.h"
#include "Managers/VulkanDeviceManager.h"
#include "Utils/VulkanEnumConverter.h"
#include "Utils/VulkanConstants.h"
#include "Utils/RenderCoreHelpers.h"
#include <boost/log/trivial.hpp>
#include <stdexcept>
#include <algorithm>
#include <set>

using namespace RenderCore;

VulkanDeviceManager VulkanDeviceManager::g_Instance{};

VulkanDeviceManager::VulkanDeviceManager()
    : m_PhysicalDevice(VK_NULL_HANDLE)
    , m_Device(VK_NULL_HANDLE)
    , m_GraphicsQueue()
    , m_PresentationQueue()
    , m_TransferQueue()
    , m_UniqueQueueFamilyIndices()
    , m_DeviceProperties()
{
}

VulkanDeviceManager::~VulkanDeviceManager()
{
	Shutdown();
}

VulkanDeviceManager &VulkanDeviceManager::Get()
{
    return g_Instance;
}

void VulkanDeviceManager::PickPhysicalDevice()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Picking a physical device";

    for (const VkPhysicalDevice &DeviceIter : GetAvailablePhysicalDevices())
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
    
    for (const char *const &RequiredLayerIter : g_RequiredDeviceLayers)
    {
        ListAvailablePhysicalDeviceLayerExtensions(RequiredLayerIter);
    }
    
    for (const char *const &DebugLayerIter : g_DebugDeviceLayers)
    {
        ListAvailablePhysicalDeviceLayerExtensions(DebugLayerIter);
    }

    ListAvailablePhysicalDeviceSurfaceCapabilities();
    ListAvailablePhysicalDeviceSurfaceFormats();
    ListAvailablePhysicalDeviceSurfacePresentationModes();
#endif
}

void VulkanDeviceManager::CreateLogicalDevice()
{
    std::optional<std::uint8_t> GraphicsQueueFamilyIndex = std::nullopt;
    std::optional<std::uint8_t> PresentationQueueFamilyIndex = std::nullopt;
    std::optional<std::uint8_t> TransferQueueFamilyIndex = std::nullopt;

    if (!GetQueueFamilyIndices(GraphicsQueueFamilyIndex, PresentationQueueFamilyIndex, TransferQueueFamilyIndex))
    {
        throw std::runtime_error("Failed to get queue family indices.");
    }

    m_GraphicsQueue.first = GraphicsQueueFamilyIndex.value();
    m_PresentationQueue.first = PresentationQueueFamilyIndex.value();
    m_TransferQueue.first = TransferQueueFamilyIndex.value();

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan logical device";

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    std::vector<const char *> Layers(g_RequiredDeviceLayers.begin(), g_RequiredDeviceLayers.end());
    std::vector<const char *> Extensions(g_RequiredDeviceExtensions.begin(), g_RequiredDeviceExtensions.end());
    
#ifdef _DEBUG
    Layers.insert(Layers.end(), g_DebugDeviceLayers.begin(), g_DebugDeviceLayers.end());
    Extensions.insert(Extensions.end(), g_DebugDeviceExtensions.begin(), g_DebugDeviceExtensions.end());
#endif

    std::unordered_map<std::uint8_t, std::uint8_t> QueueFamilyIndices;
    QueueFamilyIndices.emplace(m_GraphicsQueue.first, 1u);
    if (!QueueFamilyIndices.contains(m_PresentationQueue.first))
    {
        QueueFamilyIndices.emplace(m_PresentationQueue.first, 1u);
    }
    else
    {
        ++QueueFamilyIndices[m_PresentationQueue.first];
    }

    if (!QueueFamilyIndices.contains(m_TransferQueue.first))
    {        
        QueueFamilyIndices.emplace(m_TransferQueue.first, 1u);
    }
    else
    {
        ++QueueFamilyIndices[m_TransferQueue.first];
    }

    m_UniqueQueueFamilyIndices.clear();
    std::unordered_map<std::uint32_t, std::vector<float>> QueuePriorities;
    for (const auto &[Index, Quantity] : QueueFamilyIndices)
    {
        m_UniqueQueueFamilyIndices.push_back(Index);
        QueuePriorities.emplace(Index, std::vector<float>(Quantity, 1.0f));
    }

    std::vector<VkDeviceQueueCreateInfo> QueueCreateInfo;
    for (const auto &QueueFamilyIndex : QueueFamilyIndices)
    {
        QueueCreateInfo.push_back({
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = QueueFamilyIndex.first,
            .queueCount = QueueFamilyIndex.second,
            .pQueuePriorities = QueuePriorities.at(QueueFamilyIndex.second).data()});
    }

    const VkPhysicalDeviceFeatures DeviceFeatures{
        .samplerAnisotropy = VK_TRUE
    };

    const VkDeviceCreateInfo DeviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<std::uint32_t>(QueueCreateInfo.size()),
        .pQueueCreateInfos = QueueCreateInfo.data(),
        .enabledLayerCount = static_cast<std::uint32_t>(Layers.size()),
        .ppEnabledLayerNames = Layers.data(),
        .enabledExtensionCount = static_cast<std::uint32_t>(Extensions.size()),
        .ppEnabledExtensionNames = Extensions.data(),
        .pEnabledFeatures = &DeviceFeatures};

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateDevice(m_PhysicalDevice, &DeviceCreateInfo, nullptr, &m_Device));

    if (vkGetDeviceQueue(m_Device, m_GraphicsQueue.first, 0u, &m_GraphicsQueue.second);
        m_GraphicsQueue.second == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to get graphics queue.");
    }

    if (vkGetDeviceQueue(m_Device, m_PresentationQueue.first, 0u, &m_PresentationQueue.second);
        m_PresentationQueue.second == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to get presentation queue.");
    }

    if (vkGetDeviceQueue(m_Device, m_TransferQueue.first, 0u, &m_TransferQueue.second);
        m_TransferQueue.second == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to get transfer queue.");
    }
}

bool VulkanDeviceManager::UpdateDeviceProperties(GLFWwindow *const Window)
{
    m_DeviceProperties.Capabilities = GetAvailablePhysicalDeviceSurfaceCapabilities();

    const std::vector<VkSurfaceFormatKHR> SupportedFormats = GetAvailablePhysicalDeviceSurfaceFormats();
    if (SupportedFormats.empty())
    {
        throw std::runtime_error("No supported surface formats found.");
    }

    const std::vector<VkPresentModeKHR> SupportedPresentationModes = GetAvailablePhysicalDeviceSurfacePresentationModes();
    if (SupportedFormats.empty())
    {
        throw std::runtime_error("No supported presentation modes found.");
    }

    if (m_DeviceProperties.Capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max())
    {
        m_DeviceProperties.Extent = m_DeviceProperties.Capabilities.currentExtent;
    }
    else
    {
        m_DeviceProperties.Extent = RenderCoreHelpers::GetWindowExtent(Window, m_DeviceProperties.Capabilities);
    }

    m_DeviceProperties.Format = SupportedFormats[0];
    if (const auto MatchingFormat = std::find_if(SupportedFormats.begin(), SupportedFormats.end(),
                                                 [](const VkSurfaceFormatKHR &Iter)
                                                 {
                                                     return Iter.format == VK_FORMAT_B8G8R8A8_SRGB && Iter.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                                                 });
        MatchingFormat != SupportedFormats.end())
    {
        m_DeviceProperties.Format = *MatchingFormat;
    }

    m_DeviceProperties.Mode = VK_PRESENT_MODE_FIFO_KHR;
    if (const auto MatchingMode = std::find_if(SupportedPresentationModes.begin(), SupportedPresentationModes.end(),
                                               [](const VkPresentModeKHR &Iter)
                                               {
                                                   return Iter == VK_PRESENT_MODE_MAILBOX_KHR;
                                               });
        MatchingMode != SupportedPresentationModes.end())
    {
        m_DeviceProperties.Mode = *MatchingMode;
    }

    // find preferred depth format
    const std::vector<VkFormat> PreferredDepthFormats = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT};

    for (const VkFormat &FormatIter : PreferredDepthFormats)
    {
        VkFormatProperties FormatProperties;
        vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, FormatIter, &FormatProperties);

        if (FormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            m_DeviceProperties.DepthFormat = FormatIter;
            break;
        }
    }

    return m_DeviceProperties.IsValid();
}

VulkanDeviceProperties &VulkanDeviceManager::GetDeviceProperties()
{
    return m_DeviceProperties;
}

VkDevice &VulkanDeviceManager::GetLogicalDevice()
{
    return m_Device;
}

VkPhysicalDevice &VulkanDeviceManager::GetPhysicalDevice()
{
    return m_PhysicalDevice;
}

std::pair<std::uint8_t, VkQueue> &VulkanDeviceManager::GetGraphicsQueue()
{
    return m_GraphicsQueue;
}

std::pair<std::uint8_t, VkQueue> &VulkanDeviceManager::GetPresentationQueue()
{
    return m_PresentationQueue;
}

std::pair<std::uint8_t, VkQueue> &VulkanDeviceManager::GetTransferQueue()
{
    return m_TransferQueue; 
}

std::vector<std::uint8_t> &VulkanDeviceManager::GetUniqueQueueFamilyIndices()
{
    return m_UniqueQueueFamilyIndices;
}

std::vector<std::uint32_t> VulkanDeviceManager::GetUniqueQueueFamilyIndices_u32()
{
    std::vector<std::uint32_t> QueueFamilyIndices_u32(m_UniqueQueueFamilyIndices.size());
    std::transform(m_UniqueQueueFamilyIndices.begin(), m_UniqueQueueFamilyIndices.end(), QueueFamilyIndices_u32.begin(), [](const std::uint8_t &Index) { return static_cast<std::uint32_t>(Index); });
    return QueueFamilyIndices_u32;
}

std::uint32_t VulkanDeviceManager::GetMinImageCount() const
{
    const bool bSupportsTripleBuffering = m_DeviceProperties.Capabilities.minImageCount < 3u && m_DeviceProperties.Capabilities.maxImageCount >= 3u;
    return bSupportsTripleBuffering ? 3u : m_DeviceProperties.Capabilities.minImageCount;
}

void VulkanDeviceManager::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan device manager";

    vkDestroyDevice(m_Device, nullptr);
    m_Device = VK_NULL_HANDLE;

    m_PhysicalDevice = VK_NULL_HANDLE;
    m_GraphicsQueue.second = VK_NULL_HANDLE;
    m_PresentationQueue.second = VK_NULL_HANDLE;
    m_TransferQueue.second = VK_NULL_HANDLE;
}

bool VulkanDeviceManager::IsInitialized() const
{
    return m_PhysicalDevice != VK_NULL_HANDLE
        && m_Device != VK_NULL_HANDLE
        && m_GraphicsQueue.second != VK_NULL_HANDLE
        && m_PresentationQueue.second != VK_NULL_HANDLE
        && m_TransferQueue.second != VK_NULL_HANDLE;
}


std::vector<VkPhysicalDevice> VulkanDeviceManager::GetAvailablePhysicalDevices() const
{
    const VkInstance &VulkanInstance = VulkanRenderCore::Get().GetInstance();

    std::uint32_t DeviceCount = 0u;
    RENDERCORE_CHECK_VULKAN_RESULT(vkEnumeratePhysicalDevices(VulkanInstance, &DeviceCount, nullptr));

    std::vector<VkPhysicalDevice> Output(DeviceCount, VK_NULL_HANDLE);
    RENDERCORE_CHECK_VULKAN_RESULT(vkEnumeratePhysicalDevices(VulkanInstance, &DeviceCount, Output.data()));

    return Output;
}

std::vector<VkExtensionProperties> VulkanDeviceManager::GetAvailablePhysicalDeviceExtensions() const
{
    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    std::uint32_t ExtensionsCount;
    RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &ExtensionsCount, nullptr));

    std::vector<VkExtensionProperties> Output(ExtensionsCount);
    RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &ExtensionsCount, Output.data()));

    return Output;
}

std::vector<VkLayerProperties> VulkanDeviceManager::GetAvailablePhysicalDeviceLayers() const
{
    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    std::uint32_t LayersCount;
    RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateDeviceLayerProperties(m_PhysicalDevice, &LayersCount, nullptr));

    std::vector<VkLayerProperties> Output(LayersCount);
    RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateDeviceLayerProperties(m_PhysicalDevice, &LayersCount, Output.data()));

    return Output;
}

std::vector<VkExtensionProperties> VulkanDeviceManager::GetAvailablePhysicalDeviceLayerExtensions(const std::string_view LayerName) const
{
    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    const std::vector<std::string> AvailableLayers = GetAvailablePhysicalDeviceLayersNames();
    if (std::find(AvailableLayers.begin(), AvailableLayers.end(), LayerName) == AvailableLayers.end())
    {
        return std::vector<VkExtensionProperties>();
    }

    std::uint32_t ExtensionsCount;
    RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, LayerName.data(), &ExtensionsCount, nullptr));

    std::vector<VkExtensionProperties> Output(ExtensionsCount);
    RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, LayerName.data(), &ExtensionsCount, Output.data()));

    return Output;
}

std::vector<std::string> VulkanDeviceManager::GetAvailablePhysicalDeviceExtensionsNames() const
{    
    std::vector<std::string> Output;
    for (const VkExtensionProperties &ExtensionIter : GetAvailablePhysicalDeviceExtensions())
    {
        Output.emplace_back(ExtensionIter.extensionName);
    }

    return Output;
}

std::vector<std::string> VulkanDeviceManager::GetAvailablePhysicalDeviceLayerExtensionsNames(const std::string_view LayerName) const
{    
    std::vector<std::string> Output;
    for (const VkExtensionProperties &ExtensionIter : GetAvailablePhysicalDeviceLayerExtensions(LayerName))
    {
        Output.emplace_back(ExtensionIter.extensionName);
    }

    return Output;
}

std::vector<std::string> VulkanDeviceManager::GetAvailablePhysicalDeviceLayersNames() const
{
    std::vector<std::string> Output;
    for (const VkLayerProperties &LayerIter : GetAvailablePhysicalDeviceLayers())
    {
        Output.emplace_back(LayerIter.layerName);
    }

    return Output;
}

VkSurfaceCapabilitiesKHR VulkanDeviceManager::GetAvailablePhysicalDeviceSurfaceCapabilities() const
{
    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    VkSurfaceCapabilitiesKHR Output;
    RENDERCORE_CHECK_VULKAN_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, VulkanRenderCore::Get().GetSurface(), &Output));

    return Output;
}

std::vector<VkSurfaceFormatKHR> VulkanDeviceManager::GetAvailablePhysicalDeviceSurfaceFormats() const
{
    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    const VkSurfaceKHR &VulkanSurface = VulkanRenderCore::Get().GetSurface();

    std::uint32_t Count;
    RENDERCORE_CHECK_VULKAN_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, VulkanSurface, &Count, nullptr));

    std::vector<VkSurfaceFormatKHR> Output(Count, VkSurfaceFormatKHR());
    RENDERCORE_CHECK_VULKAN_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, VulkanSurface, &Count, Output.data()));

    return Output;
}

std::vector<VkPresentModeKHR> VulkanDeviceManager::GetAvailablePhysicalDeviceSurfacePresentationModes() const
{
    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    const VkSurfaceKHR &VulkanSurface = VulkanRenderCore::Get().GetSurface();

    std::uint32_t Count;
    RENDERCORE_CHECK_VULKAN_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, VulkanSurface, &Count, nullptr));

    std::vector<VkPresentModeKHR> Output(Count, VkPresentModeKHR());
    RENDERCORE_CHECK_VULKAN_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, VulkanSurface, &Count, Output.data()));

    return Output;
}

VkDeviceSize VulkanDeviceManager::GetMinUniformBufferOffsetAlignment() const
{
    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    VkPhysicalDeviceProperties DeviceProperties;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &DeviceProperties);

    return DeviceProperties.limits.minUniformBufferOffsetAlignment;
}

bool VulkanDeviceManager::IsPhysicalDeviceSuitable(const VkPhysicalDevice &Device) const
{
    if (Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    VkPhysicalDeviceProperties DeviceProperties;
    vkGetPhysicalDeviceProperties(Device, &DeviceProperties);
    
    VkPhysicalDeviceFeatures SupportedFeatures;
    vkGetPhysicalDeviceFeatures(Device, &SupportedFeatures);

    return DeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && SupportedFeatures.samplerAnisotropy;
}

bool VulkanDeviceManager::GetQueueFamilyIndices(std::optional<std::uint8_t> &GraphicsQueueFamilyIndex, std::optional<std::uint8_t> &PresentationQueueFamilyIndex, std::optional<std::uint8_t> &TransferQueueFamilyIndex)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Getting queue family indices";

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    const VkSurfaceKHR &VulkanSurface = VulkanRenderCore::Get().GetSurface();

    std::uint32_t QueueFamilyCount = 0u;
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &QueueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &QueueFamilyCount, QueueFamilies.data());

    for (std::uint32_t Iterator = 0u; Iterator < QueueFamilyCount; ++Iterator)
    {
        if (!GraphicsQueueFamilyIndex.has_value() && QueueFamilies[Iterator].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            GraphicsQueueFamilyIndex = static_cast<std::uint8_t>(Iterator);
        }
        else if (!TransferQueueFamilyIndex.has_value() && QueueFamilies[Iterator].queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            TransferQueueFamilyIndex = static_cast<std::uint8_t>(Iterator);
        }
        else if (!PresentationQueueFamilyIndex.has_value())
        {
            VkBool32 PresentationSupport = false;
            RENDERCORE_CHECK_VULKAN_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, Iterator, VulkanSurface, &PresentationSupport));

            if (PresentationSupport)
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
void VulkanDeviceManager::ListAvailablePhysicalDevices() const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available vulkan physical devices...";

    for (const VkPhysicalDevice &DeviceIter : GetAvailablePhysicalDevices())
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

void VulkanDeviceManager::ListAvailablePhysicalDeviceExtensions() const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available vulkan physical device extensions...";

    for (const VkExtensionProperties &ExtensionIter : GetAvailablePhysicalDeviceExtensions())
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Name: " << ExtensionIter.extensionName;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Spec Version: " << ExtensionIter.specVersion << std::endl;
    }
}

void VulkanDeviceManager::ListAvailablePhysicalDeviceLayers() const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available vulkan physical device layers...";

    for (const VkLayerProperties &LayerIter : GetAvailablePhysicalDeviceLayers())
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Name: " << LayerIter.layerName;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Spec Version: " << LayerIter.specVersion;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Implementation Version: " << LayerIter.implementationVersion;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Description: " << LayerIter.description << std::endl;
    }
}

void VulkanDeviceManager::ListAvailablePhysicalDeviceLayerExtensions(const std::string_view LayerName) const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available vulkan physical device layer '" << LayerName << "' extensions...";

    for (const VkExtensionProperties &ExtensionIter : GetAvailablePhysicalDeviceLayerExtensions(LayerName))
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Name: " << ExtensionIter.extensionName;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Spec Version: " << ExtensionIter.specVersion << std::endl;
    }
}

void VulkanDeviceManager::ListAvailablePhysicalDeviceSurfaceCapabilities() const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available vulkan physical device surface capabilities...";

    const VkSurfaceCapabilitiesKHR SurfaceCapabilities = GetAvailablePhysicalDeviceSurfaceCapabilities();

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

void VulkanDeviceManager::ListAvailablePhysicalDeviceSurfaceFormats() const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available vulkan physical device surface formats...";

    for (const VkSurfaceFormatKHR &FormatIter : GetAvailablePhysicalDeviceSurfaceFormats())
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Format: " << SurfaceFormatToString(FormatIter.format);
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Color Space: " << ColorSpaceModeToString(FormatIter.colorSpace) << std::endl;
    }
}

void VulkanDeviceManager::ListAvailablePhysicalDeviceSurfacePresentationModes() const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available vulkan physical device presentation modes...";

    for (const VkPresentModeKHR &FormatIter : GetAvailablePhysicalDeviceSurfacePresentationModes())
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Mode: " << PresentationModeToString(FormatIter);
    }
}
#endif