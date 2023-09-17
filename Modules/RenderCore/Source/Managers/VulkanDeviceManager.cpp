// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "Managers/VulkanDeviceManager.h"
#include "Managers/VulkanRenderSubsystem.h"
#include "Utils/VulkanEnumConverter.h"
#include "Utils/VulkanConstants.h"
#include "Utils/RenderCoreHelpers.h"
#include <boost/log/trivial.hpp>
#include <stdexcept>
#include <set>

using namespace RenderCore;

VulkanDeviceManager::VulkanDeviceManager()
    : m_PhysicalDevice(VK_NULL_HANDLE)
    , m_Device(VK_NULL_HANDLE)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan device manager";
}

VulkanDeviceManager::~VulkanDeviceManager()
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing vulkan device manager";
    Shutdown();
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

    VulkanRenderSubsystem::Get()->SetPhysicalDevice(m_PhysicalDevice);

#ifdef _DEBUG
    ListAvailablePhysicalDevices();
    // ListAvailablePhysicalDeviceExtensions();
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
    for (const char *const &DebugLayerIter : g_DebugDeviceLayers)
    {
        Layers.push_back(DebugLayerIter);
    }

    for (const char *const &DebugExtensionIter : g_DebugDeviceExtensions)
    {
        Extensions.push_back(DebugExtensionIter);
    }
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

    std::vector<std::uint8_t> UniqueQueueFamilyIndicesIDs;
    std::unordered_map<std::uint32_t, std::vector<float>> QueuePriorities;
    for (const auto &[Index, Quantity] : QueueFamilyIndices)
    {
        UniqueQueueFamilyIndicesIDs.push_back(Index);
        QueuePriorities.emplace(Index, std::vector<float>(Quantity, 1.0f));
    }
    VulkanRenderSubsystem::Get()->SetQueueFamilyIndices(UniqueQueueFamilyIndicesIDs);

    std::vector<VkDeviceQueueCreateInfo> QueueCreateInfo;
    QueueCreateInfo.reserve(QueueFamilyIndices.size());

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
    VulkanRenderSubsystem::Get()->SetDevice(m_Device);

    if (vkGetDeviceQueue(m_Device, m_GraphicsQueue.first, 0u, &m_GraphicsQueue.second);
        m_GraphicsQueue.second == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to get graphics queue.");
    }
    VulkanRenderSubsystem::Get()->RegisterQueue(m_GraphicsQueue.second, m_GraphicsQueue.first, VulkanQueueType::Graphics);

    if (vkGetDeviceQueue(m_Device, m_PresentationQueue.first, 0u, &m_PresentationQueue.second);
        m_PresentationQueue.second == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to get presentation queue.");
    }
    VulkanRenderSubsystem::Get()->RegisterQueue(m_PresentationQueue.second, m_PresentationQueue.first, VulkanQueueType::Present);

    if (vkGetDeviceQueue(m_Device, m_TransferQueue.first, 0u, &m_TransferQueue.second);
        m_TransferQueue.second == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to get transfer queue.");
    }
    VulkanRenderSubsystem::Get()->RegisterQueue(m_TransferQueue.second, m_TransferQueue.first, VulkanQueueType::Transfer);
}

VulkanDeviceProperties VulkanDeviceManager::GetPreferredProperties(QWindow *const Window)
{
    VulkanDeviceProperties Output;

    Output.Capabilities = GetAvailablePhysicalDeviceSurfaceCapabilities();

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

    if (Output.Capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max())
    {
        Output.Extent = Output.Capabilities.currentExtent;
    }
    else
    {
        Output.Extent = GetWindowExtent(Window, Output.Capabilities);
    }

    Output.Format = SupportedFormats[0];
    if (const auto MatchingFormat = std::find_if(SupportedFormats.begin(), SupportedFormats.end(),
                                                 [](const VkSurfaceFormatKHR &Iter)
                                                 {
                                                     return Iter.format == VK_FORMAT_B8G8R8A8_SRGB && Iter.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                                                 });
        MatchingFormat != SupportedFormats.end())
    {
        Output.Format = *MatchingFormat;
    }

    Output.Mode = VK_PRESENT_MODE_FIFO_KHR;
    if (const auto MatchingMode = std::find_if(SupportedPresentationModes.begin(), SupportedPresentationModes.end(),
                                               [](const VkPresentModeKHR &Iter)
                                               {
                                                   return Iter == VK_PRESENT_MODE_MAILBOX_KHR;
                                               });
        MatchingMode != SupportedPresentationModes.end())
    {
        Output.Mode = *MatchingMode;
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
            Output.DepthFormat = FormatIter;
            break;
        }
    }

    return Output;
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
    const VkInstance &VulkanInstance = VulkanRenderSubsystem::Get()->GetInstance();

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

std::vector<VkExtensionProperties> VulkanDeviceManager::GetAvailablePhysicalDeviceLayerExtensions(const char *LayerName) const
{
    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    std::uint32_t ExtensionsCount;
    RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, LayerName, &ExtensionsCount, nullptr));

    std::vector<VkExtensionProperties> Output(ExtensionsCount);
    RENDERCORE_CHECK_VULKAN_RESULT(vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, LayerName, &ExtensionsCount, Output.data()));

    return Output;
}

std::vector<const char *> VulkanDeviceManager::GetAvailablePhysicalDeviceExtensionsNames() const
{
    const std::vector<VkExtensionProperties> AvailableExtensions = GetAvailablePhysicalDeviceExtensions();
    
    std::vector<const char *> Output;
    Output.reserve(AvailableExtensions.size());
    for (const VkExtensionProperties &ExtensionIter : AvailableExtensions)
    {
        Output.push_back(ExtensionIter.extensionName);
    }

    return Output;
}

std::vector<const char *> VulkanDeviceManager::GetAvailablePhysicalDeviceLayersNames() const
{
    const std::vector<VkLayerProperties> AvailableLayers = GetAvailablePhysicalDeviceLayers();

    std::vector<const char *> Output;
    Output.reserve(AvailableLayers.size());
    for (const VkLayerProperties &LayerIter : AvailableLayers)
    {
        Output.push_back(LayerIter.layerName);
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
    RENDERCORE_CHECK_VULKAN_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, VulkanRenderSubsystem::Get()->GetSurface(), &Output));

    return Output;
}

std::vector<VkSurfaceFormatKHR> VulkanDeviceManager::GetAvailablePhysicalDeviceSurfaceFormats() const
{
    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    const VkSurfaceKHR &VulkanSurface = VulkanRenderSubsystem::Get()->GetSurface();

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

    const VkSurfaceKHR &VulkanSurface = VulkanRenderSubsystem::Get()->GetSurface();

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

    const VkSurfaceKHR &VulkanSurface = VulkanRenderSubsystem::Get()->GetSurface();

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

void VulkanDeviceManager::ListAvailablePhysicalDeviceLayerExtensions(const char *LayerName) const
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