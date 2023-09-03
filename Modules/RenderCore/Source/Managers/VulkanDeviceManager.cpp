// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "Managers/VulkanDeviceManager.h"
#include "Utils/VulkanEnumConverter.h"
#include "Utils/VulkanConstants.h"
#include "Utils/RenderCoreHelpers.h"
#include <boost/log/trivial.hpp>
#include <set>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

using namespace RenderCore;

VulkanDeviceManager::VulkanDeviceManager(const VkInstance &Instance, const VkSurfaceKHR &Surface)
    : m_Instance(Instance), m_Surface(Surface), m_PhysicalDevice(VK_NULL_HANDLE), m_Device(VK_NULL_HANDLE)
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

void VulkanDeviceManager::PickPhysicalDevice(const VkPhysicalDevice &PreferredDevice)
{
    if (m_Instance == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan instance is invalid.");
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Picking a physical device";

    if (PreferredDevice != VK_NULL_HANDLE && IsPhysicalDeviceSuitable(PreferredDevice))
    {
        m_PhysicalDevice = PreferredDevice;
    }
    else
    {
        for (const VkPhysicalDevice &DeviceIter : GetAvailablePhysicalDevices())
        {
            if (m_PhysicalDevice == VK_NULL_HANDLE && IsPhysicalDeviceSuitable(DeviceIter))
            {
                m_PhysicalDevice = DeviceIter;
                break;
            }
        }
    }

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("No suitable Vulkan physical device found.");
    }

#ifdef _DEBUG
    ListAvailablePhysicalDevices();
    ListAvailablePhysicalDeviceExtensions();
    ListAvailablePhysicalDeviceSurfaceCapabilities();
    ListAvailablePhysicalDeviceSurfaceFormats();
    ListAvailablePhysicalDeviceSurfacePresentationModes();
#endif
}

void VulkanDeviceManager::CreateLogicalDevice()
{
    std::optional<std::uint32_t> GraphicsQueueFamilyIndex = std::nullopt;
    std::optional<std::uint32_t> PresentationQueueFamilyIndex = std::nullopt;
    std::optional<std::uint32_t> TransferQueueFamilyIndex = std::nullopt;
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

    const std::vector<const char *> AvailableDevices = GetAvailablePhysicalDeviceExtensionsNames();
    for (const char *const &RequiredDevicesIter : g_RequiredExtensions)
    {
        if (std::find(AvailableDevices.begin(), AvailableDevices.end(), RequiredDevicesIter) != AvailableDevices.end())
        {
            const std::string ErrMessage = "Device does not supports the required extension: " + std::string(RequiredDevicesIter) + ".";
            throw std::runtime_error(ErrMessage);
        }
    }

    constexpr float QueuePriority = 1.f;

    const std::vector<VkDeviceQueueCreateInfo> QueueCreateInfo{
        VkDeviceQueueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = m_GraphicsQueue.first,
            .queueCount = 1u,
            .pQueuePriorities = &QueuePriority}};

    const VkDeviceCreateInfo DeviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<std::uint32_t>(QueueCreateInfo.size()),
        .pQueueCreateInfos = QueueCreateInfo.data(),
        .enabledExtensionCount = static_cast<std::uint32_t>(g_RequiredExtensions.size()),
        .ppEnabledExtensionNames = g_RequiredExtensions.data()};

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateDevice(m_PhysicalDevice, &DeviceCreateInfo, nullptr, &m_Device));

    if (vkGetDeviceQueue(m_Device, GetGraphicsQueueFamilyIndex(), 0u, &m_GraphicsQueue.second);
        m_GraphicsQueue.second == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to get graphics queue.");
    }

    if (vkGetDeviceQueue(m_Device, GetPresentationQueueFamilyIndex(), 0u, &m_PresentationQueue.second);
        m_PresentationQueue.second == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to get presentation queue.");
    }

    if (vkGetDeviceQueue(m_Device, GetTransferQueueFamilyIndex(), 0u, &m_TransferQueue.second);
        m_TransferQueue.second == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to get transfer queue.");
    }
}

DeviceProperties VulkanDeviceManager::GetPreferredProperties(GLFWwindow *const Window)
{
    DeviceProperties Output;

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
        Output.PreferredExtent = Output.Capabilities.currentExtent;
    }
    else
    {
        Output.PreferredExtent = GetWindowExtent(Window, Output.Capabilities);
    }

    Output.PreferredFormat = SupportedFormats[0];
    if (const auto MatchingFormat = std::find_if(SupportedFormats.begin(), SupportedFormats.end(),
                                                 [](const VkSurfaceFormatKHR &Iter)
                                                 {
                                                     return Iter.format == VK_FORMAT_B8G8R8A8_SRGB && Iter.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                                                 });
        MatchingFormat != SupportedFormats.end())
    {
        Output.PreferredFormat = *MatchingFormat;
    }

    Output.PreferredMode = VK_PRESENT_MODE_FIFO_KHR;
    if (const auto MatchingMode = std::find_if(SupportedPresentationModes.begin(), SupportedPresentationModes.end(),
                                               [](const VkPresentModeKHR &Iter)
                                               {
                                                   return Iter == VK_PRESENT_MODE_MAILBOX_KHR;
                                               });
        MatchingMode != SupportedPresentationModes.end())
    {
        Output.PreferredMode = *MatchingMode;
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
    return m_PhysicalDevice != VK_NULL_HANDLE && m_Device != VK_NULL_HANDLE && m_GraphicsQueue.second != VK_NULL_HANDLE && m_PresentationQueue.second != VK_NULL_HANDLE && m_TransferQueue.second != VK_NULL_HANDLE;
}

const VkPhysicalDevice &VulkanDeviceManager::GetPhysicalDevice() const
{
    return m_PhysicalDevice;
}

const VkDevice &VulkanDeviceManager::GetLogicalDevice() const
{
    return m_Device;
}

const VkQueue &VulkanDeviceManager::GetGraphicsQueue() const
{
    return m_GraphicsQueue.second;
}

const VkQueue &VulkanDeviceManager::GetPresentationQueue() const
{
    return m_PresentationQueue.second;
}

const VkQueue &VulkanDeviceManager::GetTransferQueue() const
{
    return m_TransferQueue.second;
}

std::vector<std::uint32_t> VulkanDeviceManager::GetQueueFamilyIndices() const
{
    std::vector<std::uint32_t> Output = {GetGraphicsQueueFamilyIndex(), GetPresentationQueueFamilyIndex(), GetTransferQueueFamilyIndex()};
    std::sort(Output.begin(), Output.end());
    Output.erase(std::unique(Output.begin(), Output.end()), Output.end());

    return Output;
}

std::uint32_t VulkanDeviceManager::GetGraphicsQueueFamilyIndex() const
{
    return m_GraphicsQueue.first;
}

std::uint32_t VulkanDeviceManager::GetPresentationQueueFamilyIndex() const
{
    return m_PresentationQueue.first;
}

std::uint32_t VulkanDeviceManager::GetTransferQueueFamilyIndex() const
{
    return m_TransferQueue.first;
}

std::vector<VkPhysicalDevice> VulkanDeviceManager::GetAvailablePhysicalDevices() const
{
    if (m_Instance == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan instance is invalid.");
    }

    std::uint32_t DeviceCount = 0u;
    RENDERCORE_CHECK_VULKAN_RESULT(vkEnumeratePhysicalDevices(m_Instance, &DeviceCount, nullptr));

    std::vector<VkPhysicalDevice> Output(DeviceCount, VkPhysicalDevice());
    RENDERCORE_CHECK_VULKAN_RESULT(vkEnumeratePhysicalDevices(m_Instance, &DeviceCount, Output.data()));

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

std::vector<const char *> VulkanDeviceManager::GetAvailablePhysicalDeviceExtensionsNames() const
{
    const std::vector<VkExtensionProperties> AvailableExtensions = GetAvailablePhysicalDeviceExtensions();
    std::set<std::string> RequiredExtensions(g_RequiredExtensions.begin(), g_RequiredExtensions.end());

    for (const VkExtensionProperties &ExtensionsIter : AvailableExtensions)
    {
        RequiredExtensions.erase(ExtensionsIter.extensionName);
    }

    std::vector<const char *> Output;
    Output.reserve(RequiredExtensions.size());
    for (const std::string &ExtensionIter : RequiredExtensions)
    {
        Output.push_back(ExtensionIter.c_str());
    }

    return Output;
}

VkSurfaceCapabilitiesKHR VulkanDeviceManager::GetAvailablePhysicalDeviceSurfaceCapabilities() const
{
    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    if (m_Surface == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan surface is invalid.");
    }

    VkSurfaceCapabilitiesKHR Output;
    RENDERCORE_CHECK_VULKAN_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &Output));

    return Output;
}

std::vector<VkSurfaceFormatKHR> VulkanDeviceManager::GetAvailablePhysicalDeviceSurfaceFormats() const
{
    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    if (m_Surface == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan surface is invalid.");
    }

    std::uint32_t Count;
    RENDERCORE_CHECK_VULKAN_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &Count, nullptr));

    std::vector<VkSurfaceFormatKHR> Output(Count, VkSurfaceFormatKHR());
    RENDERCORE_CHECK_VULKAN_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &Count, Output.data()));

    return Output;
}

std::vector<VkPresentModeKHR> VulkanDeviceManager::GetAvailablePhysicalDeviceSurfacePresentationModes() const
{
    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    if (m_Surface == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan surface is invalid.");
    }

    std::uint32_t Count;
    RENDERCORE_CHECK_VULKAN_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &Count, nullptr));

    std::vector<VkPresentModeKHR> Output(Count, VkPresentModeKHR());
    RENDERCORE_CHECK_VULKAN_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &Count, Output.data()));

    return Output;
}

bool VulkanDeviceManager::IsPhysicalDeviceSuitable(const VkPhysicalDevice &Device) const
{
    if (Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    VkPhysicalDeviceProperties DeviceProperties;
    vkGetPhysicalDeviceProperties(Device, &DeviceProperties);

    if (DeviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        return false;
    }

    return true;
}

bool VulkanDeviceManager::GetQueueFamilyIndices(std::optional<std::uint32_t> &GraphicsQueueFamilyIndex, std::optional<std::uint32_t> &PresentationQueueFamilyIndex, std::optional<std::uint32_t> &TransferQueueFamilyIndex)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Getting queue family indices";

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    if (m_Surface == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan surface is invalid.");
    }

    std::uint32_t QueueFamilyCount = 0u;
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &QueueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &QueueFamilyCount, QueueFamilies.data());

    for (std::uint32_t Iterator = 0u; Iterator < QueueFamilyCount; ++Iterator)
    {
        if (!GraphicsQueueFamilyIndex.has_value() && QueueFamilies[Iterator].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            GraphicsQueueFamilyIndex = Iterator;
        }

        if (!TransferQueueFamilyIndex.has_value() && QueueFamilies[Iterator].queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            TransferQueueFamilyIndex = Iterator;
        }

        if (!PresentationQueueFamilyIndex.has_value())
        {
            VkBool32 PresentationSupport = false;
            RENDERCORE_CHECK_VULKAN_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, Iterator, m_Surface, &PresentationSupport));

            if (PresentationSupport)
            {
                PresentationQueueFamilyIndex = Iterator;
            }
        }

        if (GraphicsQueueFamilyIndex.has_value() && PresentationQueueFamilyIndex.has_value() && TransferQueueFamilyIndex.has_value())
        {
            break;
        }
    }

    return GraphicsQueueFamilyIndex.has_value() && PresentationQueueFamilyIndex.has_value();
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

void VulkanDeviceManager::ListAvailablePhysicalDeviceSurfaceCapabilities() const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing available vulkan physical device surface capabilities...";

    const VkSurfaceCapabilitiesKHR SurfaceCapabilities = GetAvailablePhysicalDeviceSurfaceCapabilities();

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing vulkan physical device surface capabilities...";
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
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Format: " << FormatIter.format;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Color Space: " << FormatIter.colorSpace;
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