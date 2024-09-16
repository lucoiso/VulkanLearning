// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.Runtime.Device;

import RenderCore.Renderer;
import RenderCore.Runtime.SwapChain;
import RenderCore.Runtime.Instance;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import RenderCore.Utils.DebugHelpers;

using namespace RenderCore;

bool IsPhysicalDeviceSuitable(VkPhysicalDevice const &Device)
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

bool GetQueueFamilyIndices(VkSurfaceKHR const &         VulkanSurface,
                           std::optional<std::uint8_t> &GraphicsQueueFamilyIndex,
                           std::optional<std::uint8_t> &PresentationQueueFamilyIndex,
                           std::optional<std::uint8_t> &ComputeQueueFamilyIndex)
{
    std::uint32_t QueueFamilyCount = 0U;
    vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &QueueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &QueueFamilyCount, std::data(QueueFamilies));

    for (std::uint32_t Iterator = 0U; Iterator < QueueFamilyCount; ++Iterator)
    {
        if (!GraphicsQueueFamilyIndex.has_value() && (QueueFamilies.at(Iterator).queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U)
        {
            GraphicsQueueFamilyIndex.emplace(static_cast<std::uint8_t>(Iterator));

            if (!PresentationQueueFamilyIndex.has_value())
            {
                VkBool32 PresentationSupport = 0U;
                CheckVulkanResult(vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, Iterator, VulkanSurface, &PresentationSupport));

                if (PresentationSupport != 0U)
                {
                    PresentationQueueFamilyIndex.emplace(static_cast<std::uint8_t>(Iterator));
                }
            }
        }
        else if (!ComputeQueueFamilyIndex.has_value() && (QueueFamilies.at(Iterator).queueFlags & VK_QUEUE_COMPUTE_BIT) != 0U)
        {
            ComputeQueueFamilyIndex.emplace(static_cast<std::uint8_t>(Iterator));
        }

        if (GraphicsQueueFamilyIndex.has_value() && PresentationQueueFamilyIndex.has_value() && ComputeQueueFamilyIndex.has_value())
        {
            break;
        }
    }

    return GraphicsQueueFamilyIndex.has_value() && PresentationQueueFamilyIndex.has_value() && ComputeQueueFamilyIndex.has_value();
}

void PickPhysicalDevice()
{
    for (VkPhysicalDevice const &Device : GetAvailablePhysicalDevices())
    {
        if (IsPhysicalDeviceSuitable(Device))
        {
            g_PhysicalDevice = Device;
            break;
        }
    }

    vkGetPhysicalDeviceProperties(g_PhysicalDevice, &g_PhysicalDeviceProperties);
}

void CreateLogicalDevice(VkSurfaceKHR const &VulkanSurface)
{
    std::optional<std::uint8_t> GraphicsQueueFamilyIndex { std::nullopt };
    std::optional<std::uint8_t> ComputeQueueFamilyIndex { std::nullopt };
    std::optional<std::uint8_t> PresentationQueueFamilyIndex { std::nullopt };

    GetQueueFamilyIndices(VulkanSurface, GraphicsQueueFamilyIndex, PresentationQueueFamilyIndex, ComputeQueueFamilyIndex);

    std::vector Layers(std::cbegin(g_RequiredDeviceLayers), std::cend(g_RequiredDeviceLayers));
    std::vector Extensions(std::cbegin(g_RequiredDeviceExtensions), std::cend(g_RequiredDeviceExtensions));

    #ifdef _DEBUG
    Layers.insert(std::cend(Layers), std::cbegin(g_DebugDeviceLayers), std::cend(g_DebugDeviceLayers));
    Extensions.insert(std::cend(Extensions), std::cbegin(g_DebugDeviceExtensions), std::cend(g_DebugDeviceExtensions));
    #endif

    auto const AvailableLayers = GetAvailablePhysicalDeviceLayersNames();
    GetAvailableResources("device layers", Layers, g_OptionalDeviceLayers, AvailableLayers);

    auto const AvailableExtensions = GetAvailablePhysicalDeviceExtensionsNames();
    GetAvailableResources("device extensions", Extensions, g_OptionalDeviceExtensions, AvailableExtensions);

    std::unordered_map<std::uint8_t, std::uint8_t> QueueFamilyIndices { { g_GraphicsQueue.first, 1U } };

    g_UniqueQueueFamilyIndices.clear();
    g_UniqueQueueFamilyIndices.reserve(std::size(QueueFamilyIndices));

    std::vector<VkDeviceQueueCreateInfo> QueueCreateInfo;
    QueueCreateInfo.reserve(std::size(QueueFamilyIndices));

    std::vector Priorities { 0.F };
    for (auto const &Index : QueueFamilyIndices | std::views::keys)
    {
        g_UniqueQueueFamilyIndices.push_back(Index);

        QueueCreateInfo.push_back(VkDeviceQueueCreateInfo {
                                          .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                          .queueFamilyIndex = Index,
                                          .queueCount = 1U,
                                          .pQueuePriorities = std::data(Priorities)
                                  });
    }

    VkPhysicalDeviceMeshShaderFeaturesEXT MeshShaderFeatures {
            // Required
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT,
            .pNext = nullptr,
            .taskShader = VK_TRUE,
            .meshShader = VK_TRUE
    };

    VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT PipelineLibraryProperties {
            // Required
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT,
            .pNext = &MeshShaderFeatures,
            .graphicsPipelineLibrary = VK_TRUE,
    };

    VkPhysicalDeviceBufferDeviceAddressFeatures BufferDeviceAddressFeatures {
            // Required
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
            .pNext = &PipelineLibraryProperties,
            .bufferDeviceAddress = VK_TRUE
    };

    VkPhysicalDeviceDescriptorBufferFeaturesEXT DescriptorBufferFeatures {
            // Required
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT,
            .pNext = &BufferDeviceAddressFeatures,
            .descriptorBuffer = VK_TRUE
    };

    VkPhysicalDeviceSynchronization2Features Synchronization2Features {
            // Required
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
            .pNext = &DescriptorBufferFeatures,
            .synchronization2 = VK_TRUE
    };

    VkPhysicalDeviceDynamicRenderingFeatures DynamicRenderingFeatures {
            // Required
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
            .pNext = &Synchronization2Features,
            .dynamicRendering = VK_TRUE
    };

    VkPhysicalDeviceFeatures2 DeviceFeatures {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &DynamicRenderingFeatures,
            .features = VkPhysicalDeviceFeatures {
                    .independentBlend = VK_TRUE,
                    .drawIndirectFirstInstance = true,
                    .fillModeNonSolid = true,
                    .wideLines = true,
                    .samplerAnisotropy = VK_TRUE,
                    .pipelineStatisticsQuery = true,
                    .vertexPipelineStoresAndAtomics = true,
                    .fragmentStoresAndAtomics = true,
                    .shaderImageGatherExtended = true,
                    .shaderInt16 = false
            }
    };

    VkDeviceCreateInfo const DeviceCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &DeviceFeatures,
            .queueCreateInfoCount = static_cast<std::uint32_t>(std::size(QueueCreateInfo)),
            .pQueueCreateInfos = std::data(QueueCreateInfo),
            .enabledLayerCount = static_cast<std::uint32_t>(std::size(Layers)),
            .ppEnabledLayerNames = std::data(Layers),
            .enabledExtensionCount = static_cast<std::uint32_t>(std::size(Extensions)),
            .ppEnabledExtensionNames = std::data(Extensions)
    };

    CheckVulkanResult(vkCreateDevice(g_PhysicalDevice, &DeviceCreateInfo, nullptr, &g_Device));
    volkLoadDevice(g_Device);

    vkGetDeviceQueue(g_Device, g_GraphicsQueue.first, 0U, &g_GraphicsQueue.second);
}

void RenderCore::InitializeDevice(VkSurfaceKHR const &VulkanSurface)
{
    if (g_PhysicalDevice != VK_NULL_HANDLE)
    {
        return;
    }

    PickPhysicalDevice();
    CreateLogicalDevice(VulkanSurface);
}

VkSurfaceCapabilitiesKHR RenderCore::GetSurfaceCapabilities()
{
    VkSurfaceCapabilitiesKHR Output;
    CheckVulkanResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_PhysicalDevice, GetSurface(), &Output));

    return Output;
}

std::vector<std::uint32_t> RenderCore::GetUniqueQueueFamilyIndicesU32()
{
    std::vector<std::uint32_t> QueueFamilyIndicesU32(std::size(g_UniqueQueueFamilyIndices));

    std::ranges::transform(g_UniqueQueueFamilyIndices,
                           std::begin(QueueFamilyIndicesU32),
                           [](std::uint8_t const &Index)
                           {
                               return static_cast<std::uint32_t>(Index);
                           });

    return QueueFamilyIndicesU32;
}

void RenderCore::ReleaseDeviceResources()
{
    vkDestroyDevice(g_Device, nullptr);
    g_Device = VK_NULL_HANDLE;

    g_PhysicalDevice       = VK_NULL_HANDLE;
    g_GraphicsQueue.second = VK_NULL_HANDLE;
}

std::vector<VkPhysicalDevice> RenderCore::GetAvailablePhysicalDevices()
{
    VkInstance const &VulkanInstance = GetInstance();

    std::uint32_t DeviceCount = 0U;
    CheckVulkanResult(vkEnumeratePhysicalDevices(VulkanInstance, &DeviceCount, nullptr));

    std::vector<VkPhysicalDevice> Output(DeviceCount, VK_NULL_HANDLE);
    CheckVulkanResult(vkEnumeratePhysicalDevices(VulkanInstance, &DeviceCount, std::data(Output)));

    return Output;
}

std::vector<VkExtensionProperties> RenderCore::GetAvailablePhysicalDeviceExtensions()
{
    std::uint32_t ExtensionsCount = 0;
    CheckVulkanResult(vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &ExtensionsCount, nullptr));

    std::vector<VkExtensionProperties> Output(ExtensionsCount);
    CheckVulkanResult(vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &ExtensionsCount, std::data(Output)));

    return Output;
}

std::vector<VkLayerProperties> RenderCore::GetAvailablePhysicalDeviceLayers()
{
    std::uint32_t LayersCount = 0;
    CheckVulkanResult(vkEnumerateDeviceLayerProperties(g_PhysicalDevice, &LayersCount, nullptr));

    std::vector<VkLayerProperties> Output(LayersCount);
    CheckVulkanResult(vkEnumerateDeviceLayerProperties(g_PhysicalDevice, &LayersCount, std::data(Output)));

    return Output;
}

std::vector<VkExtensionProperties> RenderCore::GetAvailablePhysicalDeviceLayerExtensions(strzilla::string_view const LayerName)
{
    if (std::vector<strzilla::string> const AvailableLayers = GetAvailablePhysicalDeviceLayersNames();
        std::find(std::execution::unseq, std::cbegin(AvailableLayers), std::cend(AvailableLayers), LayerName) == std::cend(AvailableLayers))
    {
        return {};
    }

    std::uint32_t ExtensionsCount = 0;
    CheckVulkanResult(vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, std::data(LayerName), &ExtensionsCount, nullptr));

    std::vector<VkExtensionProperties> Output(ExtensionsCount);
    CheckVulkanResult(vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, std::data(LayerName), &ExtensionsCount, std::data(Output)));

    return Output;
}

std::vector<strzilla::string> RenderCore::GetAvailablePhysicalDeviceExtensionsNames()
{
    std::vector<strzilla::string> Output;
    for (auto const &[Name, Version] : GetAvailablePhysicalDeviceExtensions())
    {
        Output.emplace_back(Name);
    }

    return Output;
}

std::vector<strzilla::string> RenderCore::GetAvailablePhysicalDeviceLayerExtensionsNames(strzilla::string_view const LayerName)
{
    std::vector<strzilla::string> Output;
    for (auto const &[Name, Version] : GetAvailablePhysicalDeviceLayerExtensions(LayerName))
    {
        Output.emplace_back(Name);
    }

    return Output;
}

std::vector<strzilla::string> RenderCore::GetAvailablePhysicalDeviceLayersNames()
{
    std::vector<strzilla::string> Output;
    for (auto const &[Name, SpecVer, ImplVer, Description] : GetAvailablePhysicalDeviceLayers())
    {
        Output.emplace_back(Name);
    }

    return Output;
}

std::vector<VkSurfaceFormatKHR> RenderCore::GetAvailablePhysicalDeviceSurfaceFormats()
{
    VkSurfaceKHR const &Surface = GetSurface();

    std::uint32_t Count = 0U;
    CheckVulkanResult(vkGetPhysicalDeviceSurfaceFormatsKHR(g_PhysicalDevice, Surface, &Count, nullptr));

    std::vector Output(Count, VkSurfaceFormatKHR());
    CheckVulkanResult(vkGetPhysicalDeviceSurfaceFormatsKHR(g_PhysicalDevice, Surface, &Count, std::data(Output)));

    return Output;
}

std::vector<VkPresentModeKHR> RenderCore::GetAvailablePhysicalDeviceSurfacePresentationModes()
{
    VkSurfaceKHR const &Surface = GetSurface();

    std::uint32_t Count = 0U;
    CheckVulkanResult(vkGetPhysicalDeviceSurfacePresentModesKHR(g_PhysicalDevice, Surface, &Count, nullptr));

    std::vector Output(Count, VkPresentModeKHR());
    CheckVulkanResult(vkGetPhysicalDeviceSurfacePresentModesKHR(g_PhysicalDevice, Surface, &Count, std::data(Output)));

    return Output;
}
