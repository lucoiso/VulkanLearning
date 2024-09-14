// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Runtime.Device;

import RenderCore.Types.SurfaceProperties;

namespace RenderCore
{
    RENDERCOREMODULE_API VkPhysicalDevice                 g_PhysicalDevice { VK_NULL_HANDLE };
    RENDERCOREMODULE_API VkPhysicalDeviceProperties       g_PhysicalDeviceProperties {};
    RENDERCOREMODULE_API VkDevice                         g_Device { VK_NULL_HANDLE };
    RENDERCOREMODULE_API std::pair<std::uint8_t, VkQueue> g_GraphicsQueue {};
    RENDERCOREMODULE_API std::vector<std::uint8_t>        g_UniqueQueueFamilyIndices {};

    export void InitializeDevice(VkSurfaceKHR const &);
    export void ReleaseDeviceResources();

    export [[nodiscard]] VkSurfaceCapabilitiesKHR   GetSurfaceCapabilities();
    export [[nodiscard]] SurfaceProperties          GetSurfaceProperties(GLFWwindow *);
    export [[nodiscard]] std::vector<std::uint32_t> GetUniqueQueueFamilyIndicesU32();

    [[nodiscard]] std::vector<VkPhysicalDevice>      GetAvailablePhysicalDevices();
    [[nodiscard]] std::vector<VkExtensionProperties> GetAvailablePhysicalDeviceExtensions();
    [[nodiscard]] std::vector<VkLayerProperties>     GetAvailablePhysicalDeviceLayers();
    [[nodiscard]] std::vector<VkExtensionProperties> GetAvailablePhysicalDeviceLayerExtensions(strzilla::string_view);
    [[nodiscard]] std::vector<strzilla::string>      GetAvailablePhysicalDeviceExtensionsNames();
    [[nodiscard]] std::vector<strzilla::string>      GetAvailablePhysicalDeviceLayerExtensionsNames(strzilla::string_view);
    [[nodiscard]] std::vector<strzilla::string>      GetAvailablePhysicalDeviceLayersNames();
    [[nodiscard]] std::vector<VkSurfaceFormatKHR>    GetAvailablePhysicalDeviceSurfaceFormats();
    [[nodiscard]] std::vector<VkPresentModeKHR>      GetAvailablePhysicalDeviceSurfacePresentationModes();

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
