// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Runtime.Device;

import RenderCore.Types.SurfaceProperties;

namespace RenderCore
{
    RENDERCOREMODULE_API VkPhysicalDevice           g_PhysicalDevice{VK_NULL_HANDLE};
    RENDERCOREMODULE_API VkPhysicalDeviceProperties g_PhysicalDeviceProperties{};
    RENDERCOREMODULE_API VkDevice                   g_Device{VK_NULL_HANDLE};
    RENDERCOREMODULE_API std::pair<std::uint8_t, VkQueue> g_GraphicsQueue{};
    RENDERCOREMODULE_API std::vector<std::uint8_t> g_UniqueQueueFamilyIndices{};
    RENDERCOREMODULE_API std::function<SurfaceProperties()> g_OnGetSurfaceProperties{};

    export void InitializeDevice(VkSurfaceKHR const &);
    export void ReleaseDeviceResources();

    export RENDERCOREMODULE_API [[nodiscard]] VkSurfaceCapabilitiesKHR GetSurfaceCapabilities();
    export RENDERCOREMODULE_API [[nodiscard]] inline SurfaceProperties GetSurfaceProperties()
    {
        return g_OnGetSurfaceProperties ? g_OnGetSurfaceProperties() : SurfaceProperties{};
    }
    export RENDERCOREMODULE_API [[nodiscard]] std::vector<std::uint32_t> GetUniqueQueueFamilyIndicesU32();

    export RENDERCOREMODULE_API [[nodiscard]] std::vector<VkPhysicalDevice>      GetAvailablePhysicalDevices();
    export RENDERCOREMODULE_API [[nodiscard]] std::vector<VkExtensionProperties> GetAvailablePhysicalDeviceExtensions();
    export RENDERCOREMODULE_API [[nodiscard]] std::vector<VkLayerProperties>     GetAvailablePhysicalDeviceLayers();
    export RENDERCOREMODULE_API [[nodiscard]] std::vector<VkExtensionProperties> GetAvailablePhysicalDeviceLayerExtensions(strzilla::string_view);
    export RENDERCOREMODULE_API [[nodiscard]] std::vector<strzilla::string>      GetAvailablePhysicalDeviceExtensionsNames();
    export RENDERCOREMODULE_API [[nodiscard]] std::vector<strzilla::string>      GetAvailablePhysicalDeviceLayerExtensionsNames(strzilla::string_view);
    export RENDERCOREMODULE_API [[nodiscard]] std::vector<strzilla::string>      GetAvailablePhysicalDeviceLayersNames();
    export RENDERCOREMODULE_API [[nodiscard]] std::vector<VkSurfaceFormatKHR>    GetAvailablePhysicalDeviceSurfaceFormats();
    export RENDERCOREMODULE_API [[nodiscard]] std::vector<VkPresentModeKHR>      GetAvailablePhysicalDeviceSurfacePresentationModes();

    export RENDERCOREMODULE_API inline void SetOnGetSurfacePropertiesCallback(std::function<SurfaceProperties()> &&Callback)
    {
        g_OnGetSurfaceProperties = std::move(Callback);
    }

    export RENDERCOREMODULE_API [[nodiscard]] inline VkDevice &GetLogicalDevice()
    {
        return g_Device;
    }

    export RENDERCOREMODULE_API [[nodiscard]] inline VkPhysicalDevice &GetPhysicalDevice()
    {
        return g_PhysicalDevice;
    }

    export RENDERCOREMODULE_API [[nodiscard]] inline std::pair<std::uint8_t, VkQueue> &GetGraphicsQueue()
    {
        return g_GraphicsQueue;
    }

    export RENDERCOREMODULE_API [[nodiscard]] inline VkPhysicalDeviceProperties const &GetPhysicalDeviceProperties()
    {
        return g_PhysicalDeviceProperties;
    }
} // namespace RenderCore
