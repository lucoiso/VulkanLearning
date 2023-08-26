// Copyright Notice: [...]

#include "VulkanConfigurator.h"
#include "Window.h"
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
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Validation layer: " << CallbackData->pMessage;
    }

    return VK_FALSE;
}

static VkResult CreateDebugUtilsMessengerEXT(VkInstance Instance,
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

static void DestroyDebugUtilsMessengerEXT(VkInstance Instance,
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
    : m_Instance()
    , m_Surface()
    , m_PhysicalDevice()
    , m_Device()
    , m_GraphicsQueue()
    , m_PresentationQueue()
    , m_DebugMessenger()
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

void VulkanConfigurator::Initialize(GLFWwindow* const Window)
{
    if (IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Initializing vulkan configurator";

    if (!Window)
    {
        throw std::runtime_error("GLFW Window is invalid.");
    }

    CheckSupportedValidationLayers();
    CreateInstance();
    CreateSurface(Window);
    SetupDebugMessages();
    PickPhysicalDevice();

    std::optional<std::uint32_t> GraphicsQueueFamilyIndex = -1;
    std::optional<std::uint32_t> PresentQueueFamilyIndex = -1;
    ChooseQueueFamilyIndices(GraphicsQueueFamilyIndex, PresentQueueFamilyIndex);

    if (GraphicsQueueFamilyIndex.has_value() && PresentQueueFamilyIndex.has_value())
    {
        CreateLogicalDevice(GraphicsQueueFamilyIndex.value(), PresentQueueFamilyIndex.value());
    }
    else
    {
        throw std::runtime_error("Failed to initialize vulkan.");
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Vulkan configurator initialized";
}

void VulkanConfigurator::Shutdown()
{
    if (IsInitialized())
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan configurator";

        ShutdownDebugMessages();

        vkDestroyDevice(m_Device, nullptr);
        vkDestroyInstance(m_Instance, nullptr);

        m_Instance          = VK_NULL_HANDLE;
        m_Surface           = VK_NULL_HANDLE;
        m_PhysicalDevice    = VK_NULL_HANDLE;
        m_Device            = VK_NULL_HANDLE;
        m_GraphicsQueue     = VK_NULL_HANDLE;
        m_PresentationQueue      = VK_NULL_HANDLE;
    }
}

bool VulkanConfigurator::IsInitialized() const
{
    return m_Instance       != VK_NULL_HANDLE 
        && m_Surface        != VK_NULL_HANDLE 
        && m_PhysicalDevice != VK_NULL_HANDLE 
        && m_Device         != VK_NULL_HANDLE
        && m_GraphicsQueue  != VK_NULL_HANDLE
        && m_PresentationQueue   != VK_NULL_HANDLE;
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

std::vector<const char*> VulkanConfigurator::GetInstanceExtensions() const
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

std::vector<const char*> VulkanConfigurator::GetPhysicalDeviceExtensions() const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Getting vulkan physical device extensions";

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    std::uint32_t ExtensionsCount;
    vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &ExtensionsCount, nullptr);

    std::vector<VkExtensionProperties> Extensions(ExtensionsCount);
    vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &ExtensionsCount, Extensions.data());

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

void VulkanConfigurator::CreateInstance()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan instance";

    VkApplicationInfo AppInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "VulkanApp",
        .applicationVersion = VK_MAKE_VERSION(1u, 0u, 0u),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1u, 0u, 0u),
        .apiVersion = VK_API_VERSION_1_0
    };

    VkInstanceCreateInfo CreateInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .pApplicationInfo = &AppInfo,
        .enabledLayerCount = 0u,
    };

    std::vector<const char*> Extensions = GetInstanceExtensions();

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

void VulkanConfigurator::PickPhysicalDevice()
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

    std::vector<VkPhysicalDevice> Devices(DeviceCount);
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

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("No suitable Vulkan physical device found.");
    }
}

void VulkanConfigurator::ChooseQueueFamilyIndices(std::optional<std::uint32_t>& GraphicsQueueFamilyIndex, std::optional<std::uint32_t>& PresentationQueueFamilyIndex)
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
        if (QueueFamilies[Iterator].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            GraphicsQueueFamilyIndex = Iterator;
        }

        VkBool32 PresentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, Iterator, m_Surface, &PresentSupport);

        if (PresentSupport)
        {
            PresentationQueueFamilyIndex = Iterator;
        }

        if (GraphicsQueueFamilyIndex.has_value() && PresentationQueueFamilyIndex.has_value())
        {
            break;
        }
    }

    if (!GraphicsQueueFamilyIndex.has_value() || !PresentationQueueFamilyIndex.has_value())
    {
        throw std::runtime_error("Failed to find suitable queue families.");
    }
}

void VulkanConfigurator::CreateLogicalDevice(const std::uint32_t GraphicsQueueFamilyIndex, const std::uint32_t PresentationQueueFamilyIndex)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan logical device";

    if (m_PhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan physical device is invalid.");
    }

    const std::vector<const char*> AvailableDevices = GetPhysicalDeviceExtensions();
    for (const char* const& RequiredDevicesIter : m_RequiredDeviceExtensions)
    {
        if (std::find(AvailableDevices.begin(), AvailableDevices.end(), RequiredDevicesIter) != AvailableDevices.end())
        {
            const std::string ErrMessage = "Device does not supports the required extension: " + std::string(RequiredDevicesIter) + ".";
            throw std::runtime_error(ErrMessage);
        }
    }

    constexpr float QueuePriority = 1.0f;
    VkDeviceQueueCreateInfo QueueCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = GraphicsQueueFamilyIndex,
        .queueCount = 1u,
        .pQueuePriorities = &QueuePriority
    };

    VkDeviceCreateInfo DeviceCreateInfo {
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

    vkGetDeviceQueue(m_Device, GraphicsQueueFamilyIndex, 0u, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, PresentationQueueFamilyIndex, 0u, &m_PresentationQueue);
}

void VulkanConfigurator::CheckSupportedValidationLayers()
{
    #ifdef NDEBUG
    m_SupportsValidationLayer = false;
    return;
    #endif

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Checking validation layers";

    if (m_ValidationLayers.empty())
    {
        m_SupportsValidationLayer = false;
        return;
    }

    std::uint32_t LayersCount = 0u;
    if (vkEnumerateInstanceLayerProperties(&LayersCount, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to enumerate Vulkan Layers.");
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Found " << LayersCount << " validation layers";

    if (LayersCount == 0u)
    {
        m_SupportsValidationLayer = false;
        return;
    }

    std::vector<VkLayerProperties> Layers(LayersCount);
    if (vkEnumerateInstanceLayerProperties(&LayersCount, Layers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to get Vulkan Layers properties.");
    }

    for (const VkLayerProperties& LayerIter : Layers)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Name: " << LayerIter.layerName;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Description: " << LayerIter.description;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Spec Version: " << LayerIter.specVersion;
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Layer Implementation Version: " << LayerIter.implementationVersion << std::endl;
    }

    const std::set<std::string> RequiredLayers(m_ValidationLayers.begin(), m_ValidationLayers.end());

    m_SupportsValidationLayer = std::find_if(Layers.begin(),
        Layers.end(),
        [RequiredLayers](const VkLayerProperties& Item)
        {
            return std::find(RequiredLayers.begin(),
                             RequiredLayers.end(),
                             Item.layerName) != RequiredLayers.end();
        }
    ) != Layers.end();

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Validation layers support result: " << std::boolalpha << m_SupportsValidationLayer;
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

    if (CreateDebugUtilsMessengerEXT(m_Instance, &CreateInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS)
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

    DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
}