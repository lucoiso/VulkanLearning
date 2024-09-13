// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.Runtime.Instance;

import RenderCore.Utils.Constants;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.DebugHelpers;

using namespace RenderCore;

#ifdef _DEBUG
VkDebugUtilsMessengerEXT g_DebugMessenger{VK_NULL_HANDLE};
#endif

bool RenderCore::CreateVulkanInstance()
{
    constexpr VkApplicationInfo AppInfo {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "VulkanApp",
            .applicationVersion = VK_MAKE_VERSION(1U, 0U, 0U),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(1U, 0U, 0U),
            .apiVersion = VK_API_VERSION_1_3
    };

    VkInstanceCreateInfo CreateInfo {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .pApplicationInfo = &AppInfo,
            .enabledLayerCount = 0U
    };

    std::vector Layers(std::cbegin(g_RequiredInstanceLayers), std::cend(g_RequiredInstanceLayers));
    std::vector Extensions(std::cbegin(g_RequiredInstanceExtensions), std::cend(g_RequiredInstanceExtensions));

    // ReSharper disable once CppTooWideScopeInitStatement
    std::vector const GLFWExtensions = GetGLFWExtensions();

    for (strzilla::string const &ExtensionIter : GLFWExtensions)
    {
        Extensions.push_back(std::data(ExtensionIter));
    }

    #ifdef _DEBUG
    VkValidationFeaturesEXT const ValidationFeatures = GetInstanceValidationFeatures();
    CreateInfo.pNext                                 = &ValidationFeatures;

    Layers.insert(std::cend(Layers), std::cbegin(g_DebugInstanceLayers), std::cend(g_DebugInstanceLayers));
    Extensions.insert(std::cend(Extensions), std::cbegin(g_DebugInstanceExtensions), std::cend(g_DebugInstanceExtensions));

    VkDebugUtilsMessengerCreateInfoEXT CreateDebugInfo{};
    PopulateDebugInfo(CreateDebugInfo, nullptr);
    #endif

    auto const AvailableLayers = GetAvailableInstanceLayersNames();
    GetAvailableResources("instance layer", Layers, g_OptionalInstanceLayers, AvailableLayers);

    auto const AvailableExtensions = GetAvailableInstanceExtensionsNames();
    GetAvailableResources("instance extension", Extensions, g_OptionalInstanceExtensions, AvailableExtensions);

    CreateInfo.enabledLayerCount   = static_cast<std::uint32_t>(std::size(Layers));
    CreateInfo.ppEnabledLayerNames = std::data(Layers);

    CreateInfo.enabledExtensionCount   = static_cast<std::uint32_t>(std::size(Extensions));
    CreateInfo.ppEnabledExtensionNames = std::data(Extensions);

    CheckVulkanResult(vkCreateInstance(&CreateInfo, nullptr, &g_Instance));
    volkLoadInstance(g_Instance);

    #ifdef _DEBUG
    CheckVulkanResult(CreateDebugUtilsMessenger(g_Instance, &CreateDebugInfo, nullptr, &g_DebugMessenger));
    #endif

    return g_Instance != VK_NULL_HANDLE;
}

void RenderCore::DestroyVulkanInstance()
{
    #ifdef _DEBUG
    if (g_DebugMessenger != VK_NULL_HANDLE)
    {
        DestroyDebugUtilsMessenger(g_Instance, g_DebugMessenger, nullptr);
        g_DebugMessenger = VK_NULL_HANDLE;
    }
    #endif

    vkDestroyInstance(g_Instance, nullptr);
    g_Instance = VK_NULL_HANDLE;
}
