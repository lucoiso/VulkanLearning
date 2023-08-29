// Copyright Notice: [...]

#include "VulkanDeviceManager.h"
#include "VulkanEnumConverter.h"
#include "VulkanConstants.h"
#include "RenderCoreHelpers.h"
#include <boost/log/trivial.hpp>
#include <set>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

using namespace RenderCore;

VulkanDeviceManager::VulkanDeviceManager(const VkInstance& Instance, const VkSurfaceKHR& Surface)
    : m_Instance(Instance)
    , m_Surface(Surface)
    , m_PhysicalDevice(VK_NULL_HANDLE)
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

void VulkanDeviceManager::PickPhysicalDevice(const VkPhysicalDevice& PreferredDevice)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Picking a physical device";

    if (PreferredDevice != VK_NULL_HANDLE && IsDeviceSuitable(PreferredDevice))
    {
        m_PhysicalDevice = PreferredDevice;
    }
    else
    {
        if (m_Instance == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan instance is invalid.");
        }

        std::uint32_t DeviceCount = 0u;
        vkEnumeratePhysicalDevices(m_Instance, &DeviceCount, nullptr);

        if (DeviceCount == 0u)
        {
            throw std::runtime_error("No suitable Vulkan physical devices found.");
        }

        std::vector<VkPhysicalDevice> Devices(DeviceCount, VkPhysicalDevice());
        vkEnumeratePhysicalDevices(m_Instance, &DeviceCount, Devices.data());
    
        for (const VkPhysicalDevice& Device : Devices)
        {
            if (m_PhysicalDevice == VK_NULL_HANDLE && IsDeviceSuitable(Device))
            {
                m_PhysicalDevice = Device;

    #ifdef NDEBUG
                break;
    #endif
            }
        }
    }

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("No suitable Vulkan physical device found.");
    }
}

void VulkanDeviceManager::CreateLogicalDevice()
{
    std::optional<std::uint32_t> GraphicsQueueFamilyIndex = std::nullopt;
    std::optional<std::uint32_t> PresentationQueueFamilyIndex = std::nullopt;
    if (!GetQueueFamilyIndices(GraphicsQueueFamilyIndex, PresentationQueueFamilyIndex))
    {
        throw std::runtime_error("Failed to get queue family indices.");
    }

    m_GraphicsQueue.first = GraphicsQueueFamilyIndex.value();
    m_PresentationQueue.first = PresentationQueueFamilyIndex.value();

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan logical device";

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    const std::vector<const char*> AvailableDevices = GetAvailablePhysicalDeviceExtensions();
    for (const char* const& RequiredDevicesIter : g_RequiredExtensions)
    {
        if (std::find(AvailableDevices.begin(), AvailableDevices.end(), RequiredDevicesIter) != AvailableDevices.end())
        {
            const std::string ErrMessage = "Device does not supports the required extension: " + std::string(RequiredDevicesIter) + ".";
            throw std::runtime_error(ErrMessage);
        }
    }

    constexpr float QueuePriority = 1.0f;
    const VkDeviceQueueCreateInfo QueueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = GraphicsQueueFamilyIndex.value(),
        .queueCount = 1u,
        .pQueuePriorities = &QueuePriority
    };

    const VkDeviceCreateInfo DeviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1u,
        .pQueueCreateInfos = &QueueCreateInfo,
        .enabledExtensionCount = static_cast<std::uint32_t>(g_RequiredExtensions.size()),
        .ppEnabledExtensionNames = g_RequiredExtensions.data()
    };

    if (vkCreateDevice(m_PhysicalDevice, &DeviceCreateInfo, nullptr, &m_Device) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan device.");
    }

    if (vkGetDeviceQueue(m_Device, GraphicsQueueFamilyIndex.value(), 0u, &m_GraphicsQueue.second);
        m_GraphicsQueue.second == VK_NULL_HANDLE)
    {
       throw std::runtime_error("Failed to get graphics queue.");
    }

    if (vkGetDeviceQueue(m_Device, PresentationQueueFamilyIndex.value(), 0u, &m_PresentationQueue.second);
        m_PresentationQueue.second == VK_NULL_HANDLE)
    {
       throw std::runtime_error("Failed to get presentation queue.");
    }
}

void VulkanDeviceManager::GetSwapChainPreferredProperties(GLFWwindow* const Window, VkSurfaceFormatKHR& PreferredFormat, VkPresentModeKHR& PreferredMode, VkExtent2D& PreferredExtent, VkSurfaceCapabilitiesKHR& Capabilities)
{
    Capabilities = GetAvailablePhysicalDeviceSurfaceCapabilities();

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

    if (Capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max())
    {
        PreferredExtent = Capabilities.currentExtent;
    }
    else
    {
        PreferredExtent = GetWindowExtent(Window, Capabilities);
    }

    PreferredFormat = SupportedFormats[0];
    if (const auto MatchingFormat = std::find_if(SupportedFormats.begin(), SupportedFormats.end(),
            [] (const VkSurfaceFormatKHR& Iter)
            {
                return Iter.format == VK_FORMAT_B8G8R8A8_SRGB && Iter.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
            }
        );
        MatchingFormat != SupportedFormats.end())
    {
       PreferredFormat = *MatchingFormat;
    }

    PreferredMode = VK_PRESENT_MODE_FIFO_KHR;
    if (const auto MatchingMode = std::find_if(SupportedPresentationModes.begin(), SupportedPresentationModes.end(),
            [](const VkPresentModeKHR& Iter)
            {
                return Iter == VK_PRESENT_MODE_MAILBOX_KHR;
            }
        );
        MatchingMode != SupportedPresentationModes.end())
    {
        PreferredMode = *MatchingMode;
    }
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
}

bool VulkanDeviceManager::IsInitialized() const
{
    return m_Instance                   != VK_NULL_HANDLE 
        && m_Surface                    != VK_NULL_HANDLE 
        && m_PhysicalDevice             != VK_NULL_HANDLE 
        && m_Device                     != VK_NULL_HANDLE
        && m_GraphicsQueue.second       != VK_NULL_HANDLE
        && m_PresentationQueue.second   != VK_NULL_HANDLE;
}

const VkDevice& VulkanDeviceManager::GetLogicalDevice() const
{
    return m_Device;
}

const VkPhysicalDevice& VulkanDeviceManager::GetPhysicalDevice() const
{
    return m_PhysicalDevice;
}

VkSurfaceCapabilitiesKHR VulkanDeviceManager::GetAvailablePhysicalDeviceSurfaceCapabilities() const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Getting vulkan physical device surface capabilities";

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    if (m_Surface == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan surface is invalid.");
    }

    VkSurfaceCapabilitiesKHR Output;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &Output) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to get vulkan physical device surface capabilites.");
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing vulkan physical device surface capabilities...";
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Min Image Count: " << Output.minImageCount;
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Max Image Count: " << Output.maxImageCount;
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Current Extent: (" << Output.currentExtent.width << ", " << Output.currentExtent.height << ")";
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Min Image Extent: (" << Output.minImageExtent.width << ", " << Output.minImageExtent.height << ")";
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Max Image Extent: (" << Output.maxImageExtent.width << ", " << Output.maxImageExtent.height << ")";
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Max Image Array Layers: " << Output.maxImageArrayLayers;
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Supported Transforms: " << TransformFlagToString(static_cast<VkSurfaceTransformFlagBitsKHR>(Output.supportedTransforms));
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Current Transform: " << TransformFlagToString(Output.currentTransform);
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Supported Composite Alpha: " << CompositeAlphaFlagToString(Output.supportedCompositeAlpha);
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Supported Usage Flags: " << ImageUsageFlagToString(static_cast<VkImageUsageFlagBits>(Output.supportedUsageFlags));

    return Output;
}

std::vector<VkSurfaceFormatKHR> VulkanDeviceManager::GetAvailablePhysicalDeviceSurfaceFormats() const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Getting vulkan physical device surface formats";

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    if (m_Surface == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan surface is invalid.");
    }

    std::uint32_t Count;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &Count, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to get physical device surface formats");
    }

    std::vector<VkSurfaceFormatKHR> Output(Count, VkSurfaceFormatKHR());
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &Count, Output.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to get physical device surface formats after count");
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing vulkan physical device surface formats...";
    for (const VkSurfaceFormatKHR& FormatIter : Output)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Color Space: " << ColorSpaceModeToString(FormatIter.colorSpace);
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Format: " << SurfaceFormatToString(FormatIter.format) << std::endl;
    }

    return Output;
}

std::vector<VkPresentModeKHR> VulkanDeviceManager::GetAvailablePhysicalDeviceSurfacePresentationModes() const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Getting vulkan physical device surface presentation modes";

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    if (m_Surface == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan surface is invalid.");
    }

    std::uint32_t Count;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &Count, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to get physical device surface presentation modes");
    }

    std::vector<VkPresentModeKHR> Output(Count, VkPresentModeKHR());
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &Count, Output.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to get physical device surface presentation modes after count");
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Listing vulkan physical device surface presentation modes...";
    for (const VkPresentModeKHR& FormatIter : Output)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Mode: " << PresentationModeToString(FormatIter);
    }

    return Output;
}

std::vector<std::uint32_t> RenderCore::VulkanDeviceManager::GetQueueFamilyIndices() const
{
    return { m_GraphicsQueue.first, m_PresentationQueue.first };
}

std::vector<const char*> VulkanDeviceManager::GetAvailablePhysicalDeviceExtensions() const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Getting vulkan physical device extensions";

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    std::uint32_t ExtensionsCount;
    if (vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &ExtensionsCount, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to enumerate device extension properties");
    }

    std::vector<VkExtensionProperties> Extensions(ExtensionsCount);
    if (vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &ExtensionsCount, Extensions.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to enumerate device extension properties after count");
    }

    std::set<std::string> RequiredExtensions(g_RequiredExtensions.begin(), g_RequiredExtensions.end());

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Found extensions:";
    for (const VkExtensionProperties& ExtensionsIter : Extensions)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: " << ExtensionsIter.extensionName;
        
        RequiredExtensions.erase(ExtensionsIter.extensionName);
    }
    
    std::vector<const char*> Output;
    Output.reserve(RequiredExtensions.size());
    for (const std::string& ExtensionIter : RequiredExtensions)
    {
        Output.push_back(ExtensionIter.c_str());
    }

    return Output;
}

bool VulkanDeviceManager::IsDeviceSuitable(const VkPhysicalDevice& Device) const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Checking if device is suitable...";

    if (Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    VkPhysicalDeviceProperties DeviceProperties;
    VkPhysicalDeviceFeatures DeviceFeatures;

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Getting target properties...";

    vkGetPhysicalDeviceProperties(Device, &DeviceProperties);
    vkGetPhysicalDeviceFeatures(Device, &DeviceFeatures);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Name: " << DeviceProperties.deviceName;
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target ID: " << DeviceProperties.deviceID;
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Vendor ID: " << DeviceProperties.vendorID;
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target Driver Version: " << DeviceProperties.driverVersion;
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target API Version: " << DeviceProperties.apiVersion << std::endl;

    if (DeviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        return false;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Target is suitable";

    return true;
}

bool VulkanDeviceManager::GetQueueFamilyIndices(std::optional<std::uint32_t>& GraphicsQueueFamilyIndex, std::optional<std::uint32_t>& PresentationQueueFamilyIndex)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Choosing queue family indices (Graphics Queue & Presentation Queue)";

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

        if (!PresentationQueueFamilyIndex.has_value())
        {
            VkBool32 PresentationSupport = false;
            if (vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, Iterator, m_Surface, &PresentationSupport) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to check if presentation is supported.");
            }

            if (PresentationSupport)
            {
                PresentationQueueFamilyIndex = Iterator;
            }
        }

        if (GraphicsQueueFamilyIndex.has_value() && PresentationQueueFamilyIndex.has_value())
        {
            break;
        }
    }

    return GraphicsQueueFamilyIndex.has_value() && PresentationQueueFamilyIndex.has_value();
}