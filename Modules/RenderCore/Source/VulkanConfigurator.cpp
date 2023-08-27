// Copyright Notice: [...]

#include "VulkanConfigurator.h"
#include "VulkanEnumConverter.h"
#include <boost/log/trivial.hpp>
#include <iostream>
#include <stdexcept>
#include <set>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL ValidationLayerDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
                                                                   [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT MessageType,
                                                                   const VkDebugUtilsMessengerCallbackDataEXT* const CallbackData,
                                                                   [[maybe_unused]] void* UserData)
{
    #ifdef NDEBUG
    if (MessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    #endif
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Message: " << CallbackData->pMessage;
    }

    return VK_FALSE;
}

static VkResult CreateDebugUtilsMessenger(VkInstance Instance,
                                          const VkDebugUtilsMessengerCreateInfoEXT* const CreateInfo,
                                          const VkAllocationCallbacks* const Allocator,
                                          VkDebugUtilsMessengerEXT* const DebugMessenger)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating debug messenger";

    if (const auto CreationFunctor = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT")))
    {
        return CreationFunctor(Instance, CreateInfo, Allocator, DebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static void DestroyDebugUtilsMessenger(VkInstance Instance,
                                       VkDebugUtilsMessengerEXT DebugMessenger,
                                       const VkAllocationCallbacks* const Allocator)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying debug messenger";

    if (const auto DestructionFunctor = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT")))
    {
        DestructionFunctor(Instance, DebugMessenger, Allocator);
    }
}

using namespace RenderCore;

VulkanConfigurator::VulkanConfigurator()
    : m_Instance(VK_NULL_HANDLE)
    , m_Surface(VK_NULL_HANDLE)
    , m_PhysicalDevice(VK_NULL_HANDLE)
    , m_Device(VK_NULL_HANDLE)
    , m_SwapChain(VK_NULL_HANDLE)
    , m_SwapChainImages()
    , m_SwapChainImageViews()
    , m_GraphicsQueue(0u, VK_NULL_HANDLE)
    , m_PresentationQueue(0u, VK_NULL_HANDLE)
    , m_DebugMessenger(VK_NULL_HANDLE)
    , m_ValidationLayers({ "VK_LAYER_KHRONOS_validation" })
    , m_RequiredDeviceExtensions({ VK_KHR_SWAPCHAIN_EXTENSION_NAME })
    , m_SupportsValidationLayer(false)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan configurator";
}

VulkanConfigurator::~VulkanConfigurator()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing vulkan configurator";

    Shutdown();
}

void VulkanConfigurator::CreateInstance()
{
    UpdateSupportsValidationLayer();

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan instance";

    VkApplicationInfo AppInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "VulkanApp",
        .applicationVersion = VK_MAKE_VERSION(1u, 0u, 0u),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1u, 0u, 0u),
        .apiVersion = VK_API_VERSION_1_0
    };

    VkInstanceCreateInfo CreateInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .pApplicationInfo = &AppInfo,
        .enabledLayerCount = 0u,
    };

    std::vector<const char*> Extensions = GetAvailableInstanceExtensions();

    if (SupportsValidationLayer())
    {
        Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Activating validation layers in vulkan instance";
        for (const char* const& ValidationLayerIter : m_ValidationLayers)
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Including Layer: " << ValidationLayerIter;
        }

        CreateInfo.enabledLayerCount = static_cast<std::uint32_t>(m_ValidationLayers.size());
        CreateInfo.ppEnabledLayerNames = m_ValidationLayers.data();

        VkDebugUtilsMessengerCreateInfoEXT CreateDebugInfo{};
        PopulateDebugInfo(CreateDebugInfo);
        CreateInfo.pNext = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&CreateDebugInfo);
    }

    CreateInfo.enabledExtensionCount = static_cast<std::uint32_t>(Extensions.size());
    CreateInfo.ppEnabledExtensionNames = Extensions.data();

    if (vkCreateInstance(&CreateInfo, nullptr, &m_Instance) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan instance.");
    }

    SetupDebugMessages();
}

void VulkanConfigurator::UpdateSupportsValidationLayer()
{
    const std::set<std::string> RequiredLayers(m_ValidationLayers.begin(), m_ValidationLayers.end());
    const std::vector<VkLayerProperties> AvailableLayers = GetAvailableValidationLayers();

    m_SupportsValidationLayer = std::find_if(AvailableLayers.begin(),
        AvailableLayers.end(),
        [RequiredLayers](const VkLayerProperties& Item)
        {
            return std::find(RequiredLayers.begin(),
            RequiredLayers.end(),
            Item.layerName) != RequiredLayers.end();
        }
    ) != AvailableLayers.end();
}

void VulkanConfigurator::CreateSurface(GLFWwindow* const Window)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan surface";

    if (!Window)
    {
        throw std::runtime_error("GLFW Window is invalid.");
    }

    if (m_Instance == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan instance is invalid.");
    }

    if (glfwCreateWindowSurface(m_Instance, Window, nullptr, &m_Surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create window surface.");
    }
}

void VulkanConfigurator::PickPhysicalDevice(const VkPhysicalDevice& PreferredDevice)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Picking a physical device";

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

    if (PreferredDevice != VK_NULL_HANDLE && IsDeviceSuitable(PreferredDevice))
    {
        m_PhysicalDevice = PreferredDevice;
    }
    else
    {
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

void VulkanConfigurator::CreateLogicalDevice()
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
    for (const char* const& RequiredDevicesIter : m_RequiredDeviceExtensions)
    {
        if (std::find(AvailableDevices.begin(), AvailableDevices.end(), RequiredDevicesIter) != AvailableDevices.end())
        {
            const std::string ErrMessage = "Device does not supports the required extension: " + std::string(RequiredDevicesIter) + ".";
            throw std::runtime_error(ErrMessage);
        }
    }

    constexpr float QueuePriority = 1.0f;
    VkDeviceQueueCreateInfo QueueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = GraphicsQueueFamilyIndex.value(),
        .queueCount = 1u,
        .pQueuePriorities = &QueuePriority
    };

    VkDeviceCreateInfo DeviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1u,
        .pQueueCreateInfos = &QueueCreateInfo,
        .enabledExtensionCount = static_cast<std::uint32_t>(m_RequiredDeviceExtensions.size()),
        .ppEnabledExtensionNames = m_RequiredDeviceExtensions.data()
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

void VulkanConfigurator::InitializeSwapChain(GLFWwindow* const Window)
{
    const VkSurfaceCapabilitiesKHR SupportedCapabilities = GetAvailablePhysicalDeviceSurfaceCapabilities();

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

    VkExtent2D PreferredExtent;
    if (SupportedCapabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max())
    {
        PreferredExtent = SupportedCapabilities.currentExtent;
    }
    else
    {
        PreferredExtent = GetExtent(Window, SupportedCapabilities);
    }

    VkSurfaceFormatKHR PreferredFormat = SupportedFormats[0];
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

    VkPresentModeKHR PreferredMode = VK_PRESENT_MODE_FIFO_KHR;
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

    InitializeSwapChain(PreferredFormat, PreferredMode, PreferredExtent, SupportedCapabilities);
}

void VulkanConfigurator::InitializeSwapChain(const VkSurfaceFormatKHR& PreferredFormat, const VkPresentModeKHR& PreferredMode, const VkExtent2D& PreferredExtent, const VkSurfaceCapabilitiesKHR& Capabilities)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan swap chain";

    ResetSwapChain();

    if (m_Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan logical device is invalid.");
    }

    VkSwapchainCreateInfoKHR SwapChainCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = m_Surface,
        .minImageCount = Capabilities.minImageCount,
        .imageFormat = PreferredFormat.format,
        .imageColorSpace = PreferredFormat.colorSpace,
        .imageExtent = PreferredExtent,
        .imageArrayLayers = 1u,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    };

    if (m_GraphicsQueue.first != m_PresentationQueue.first)
    {
        const std::vector<std::uint32_t> QueueFamilyIndices = { m_GraphicsQueue.first, m_PresentationQueue.first };

        SwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        SwapChainCreateInfo.queueFamilyIndexCount = 2u;
        SwapChainCreateInfo.pQueueFamilyIndices = QueueFamilyIndices.data();
    }
    else
    {
        SwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        SwapChainCreateInfo.queueFamilyIndexCount = 0u;
        SwapChainCreateInfo.pQueueFamilyIndices = nullptr;
    }

    SwapChainCreateInfo.preTransform = Capabilities.currentTransform;
    SwapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    SwapChainCreateInfo.presentMode = PreferredMode;
    SwapChainCreateInfo.clipped = VK_TRUE;
    SwapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_Device, &SwapChainCreateInfo, nullptr, &m_SwapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan swap chain.");
    }

    std::uint32_t Count = 0u;
    if (vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &Count, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to get Vulkan swap chain images count.");
    }

    m_SwapChainImages.resize(Count, VK_NULL_HANDLE);
    if (vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &Count, m_SwapChainImages.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to get Vulkan swap chain images.");
    }

    CreateSwapChainImageViews(PreferredFormat.format);
}

void VulkanConfigurator::Shutdown()
{
    if (IsInitialized())
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan configurator";

        ShutdownDebugMessages();

        ResetSwapChain(false);
        vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
        vkDestroyDevice(m_Device, nullptr);
        vkDestroyInstance(m_Instance, nullptr);

        m_Instance                  = VK_NULL_HANDLE;
        m_Surface                   = VK_NULL_HANDLE;
        m_PhysicalDevice            = VK_NULL_HANDLE;
        m_Device                    = VK_NULL_HANDLE;
        m_SwapChain                 = VK_NULL_HANDLE;
        m_GraphicsQueue.second      = VK_NULL_HANDLE;
        m_PresentationQueue.second  = VK_NULL_HANDLE;
    }
}

bool VulkanConfigurator::IsInitialized() const
{
    return m_Instance                   != VK_NULL_HANDLE 
        && m_Surface                    != VK_NULL_HANDLE 
        && m_PhysicalDevice             != VK_NULL_HANDLE 
        && m_Device                     != VK_NULL_HANDLE
        && m_SwapChain                  != VK_NULL_HANDLE
        && m_GraphicsQueue.second       != VK_NULL_HANDLE
        && m_PresentationQueue.second   != VK_NULL_HANDLE;
}

VkInstance VulkanConfigurator::GetInstance() const
{
    return m_Instance;
}

VkDevice VulkanConfigurator::GetLogicalDevice() const
{
    return m_Device;
}

VkPhysicalDevice VulkanConfigurator::GetPhysicalDevice() const
{
    return m_PhysicalDevice;
}

VkExtent2D VulkanConfigurator::GetExtent(GLFWwindow* const Window) const
{
    return GetExtent(Window, GetAvailablePhysicalDeviceSurfaceCapabilities());
}

VkExtent2D VulkanConfigurator::GetExtent(GLFWwindow* const Window, const VkSurfaceCapabilitiesKHR& Capabilities) const
{
    std::int32_t Width = 0u;
    std::int32_t Height = 0u;
    glfwGetFramebufferSize(Window, &Width, &Height);

    VkExtent2D ActualExtent{
        .width = static_cast<std::uint32_t>(Width),
        .height = static_cast<std::uint32_t>(Height)
    };

    ActualExtent.width = std::clamp(ActualExtent.width, Capabilities.minImageExtent.width, Capabilities.maxImageExtent.width);
    ActualExtent.height = std::clamp(ActualExtent.height, Capabilities.minImageExtent.height, Capabilities.maxImageExtent.height);
    return VkExtent2D();
}

VkSurfaceCapabilitiesKHR VulkanConfigurator::GetAvailablePhysicalDeviceSurfaceCapabilities() const
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

std::vector<VkSurfaceFormatKHR> VulkanConfigurator::GetAvailablePhysicalDeviceSurfaceFormats() const
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

std::vector<VkPresentModeKHR> VulkanConfigurator::GetAvailablePhysicalDeviceSurfacePresentationModes() const
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

std::vector<const char*> VulkanConfigurator::GetAvailableInstanceExtensions() const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Getting vulkan instance extensions";

    std::uint32_t GLFWExtensionsCount = 0u;
    const char** const GLFWExtensions = glfwGetRequiredInstanceExtensions(&GLFWExtensionsCount);

    const std::vector<const char*> Output(GLFWExtensions, GLFWExtensions + GLFWExtensionsCount);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Found extensions:";

    for (const char* const& ExtensionIter : Output)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: " << ExtensionIter;
    }

    return Output;
}

std::vector<const char*> VulkanConfigurator::GetAvailablePhysicalDeviceExtensions() const
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

    std::set<std::string> RequiredExtensions(m_RequiredDeviceExtensions.begin(), m_RequiredDeviceExtensions.end());

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

std::vector<VkLayerProperties> VulkanConfigurator::GetAvailableValidationLayers() const
{
#ifdef NDEBUG
    m_SupportsValidationLayer = false;
    return;
#endif

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Checking available validation layers";

    std::vector<VkLayerProperties> Output;

    if (m_ValidationLayers.empty())
    {
        return Output;
    }

    std::uint32_t LayersCount = 0u;
    if (vkEnumerateInstanceLayerProperties(&LayersCount, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to enumerate Vulkan Layers.");
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Found " << LayersCount << " validation layers";

    if (LayersCount == 0u)
    {
        return Output;
    }

    Output.resize(LayersCount);
    if (vkEnumerateInstanceLayerProperties(&LayersCount, Output.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to get Vulkan Layers properties.");
    }

    for (const VkLayerProperties& LayerIter : Output)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Name: " << LayerIter.layerName;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Description: " << LayerIter.description;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Spec Version: " << LayerIter.specVersion;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Implementation Version: " << LayerIter.implementationVersion << std::endl;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Validation layers support result: " << std::boolalpha << m_SupportsValidationLayer;
    return Output;
}

bool VulkanConfigurator::IsDeviceSuitable(const VkPhysicalDevice& Device) const
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

bool VulkanConfigurator::SupportsValidationLayer() const
{
    return m_SupportsValidationLayer;
}

bool VulkanConfigurator::GetQueueFamilyIndices(std::optional<std::uint32_t>& GraphicsQueueFamilyIndex, std::optional<std::uint32_t>& PresentationQueueFamilyIndex)
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

void VulkanConfigurator::PopulateDebugInfo(VkDebugUtilsMessengerCreateInfoEXT& Info)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Populating debug info";

    Info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    Info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    Info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    Info.pfnUserCallback = ValidationLayerDebugCallback;
    Info.pUserData = reinterpret_cast<void*>(this);
}

void VulkanConfigurator::SetupDebugMessages()
{
    if (!SupportsValidationLayer())
    {
        return;
    }

    if (m_Instance == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan instance is invalid.");
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Setting up debug messages";

    VkDebugUtilsMessengerCreateInfoEXT CreateInfo{};
    PopulateDebugInfo(CreateInfo);

    if (CreateDebugUtilsMessenger(m_Instance, &CreateInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void VulkanConfigurator::ShutdownDebugMessages()
{
    if (!SupportsValidationLayer() || m_DebugMessenger == VK_NULL_HANDLE)
    {
        return;
    }

    if (m_Instance == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan instance is invalid.");
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down debug messages";

    DestroyDebugUtilsMessenger(m_Instance, m_DebugMessenger, nullptr);
}

void VulkanConfigurator::ResetSwapChain(const bool bDestroyImages)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Reseting Vulkan swap chain";

    if (m_Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan logical device is invalid.");
    }

    if (m_SwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
    }

    if (bDestroyImages)
    {
        for (const VkImage& ImageIter : m_SwapChainImages)
        {
            if (ImageIter != VK_NULL_HANDLE)
            {
                vkDestroyImage(m_Device, ImageIter, nullptr);
            }
        }
    }
    m_SwapChainImages.clear();

    for (const VkImageView& ImageViewIter : m_SwapChainImageViews)
    {
        if (ImageViewIter != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_Device, ImageViewIter, nullptr);
        }
    }
    m_SwapChainImageViews.clear();
}

void VulkanConfigurator::CreateSwapChainImageViews(const VkFormat& ImageFormat)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan image views";

    if (m_Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan logical device is invalid.");
    }

    m_SwapChainImageViews.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);
    for (auto ImageIter = m_SwapChainImages.begin(); ImageIter != m_SwapChainImages.end(); ++ImageIter)
    {
        VkImageViewCreateInfo ImageViewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = *ImageIter,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = ImageFormat,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0u,
                .levelCount = 1u,
                .baseArrayLayer = 0u,
                .layerCount = 1u
            }
        };

        const std::int32_t Index = std::distance(m_SwapChainImages.begin(), ImageIter);
        if (vkCreateImageView(m_Device, &ImageViewCreateInfo, nullptr, &m_SwapChainImageViews[Index]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan image view.");
        }
    }
}