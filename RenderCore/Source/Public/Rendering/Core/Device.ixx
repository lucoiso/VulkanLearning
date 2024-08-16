// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Runtime.Device;

import RenderCore.Types.SurfaceProperties;

namespace RenderCore
{
    export void InitializeDevice(VkSurfaceKHR const &);
    export void ReleaseDeviceResources();

    export [[nodiscard]] VkSurfaceCapabilitiesKHR GetSurfaceCapabilities();
    export [[nodiscard]] SurfaceProperties GetSurfaceProperties(GLFWwindow *);
    export [[nodiscard]] VkDevice &GetLogicalDevice();
    export [[nodiscard]] VkPhysicalDevice &GetPhysicalDevice();
    export [[nodiscard]] std::pair<std::uint8_t, VkQueue> &GetGraphicsQueue();
    export [[nodiscard]] std::vector<std::uint32_t> GetUniqueQueueFamilyIndicesU32();
    export [[nodiscard]] VkPhysicalDeviceProperties const &GetPhysicalDeviceProperties();

    [[nodiscard]] std::vector<VkPhysicalDevice> GetAvailablePhysicalDevices();

    [[nodiscard]] std::vector<VkExtensionProperties> GetAvailablePhysicalDeviceExtensions();
    [[nodiscard]] std::vector<VkLayerProperties> GetAvailablePhysicalDeviceLayers();
    [[nodiscard]] std::vector<VkExtensionProperties> GetAvailablePhysicalDeviceLayerExtensions(strzilla::string_view);
    [[nodiscard]] std::vector<strzilla::string> GetAvailablePhysicalDeviceExtensionsNames();
    [[nodiscard]] std::vector<strzilla::string> GetAvailablePhysicalDeviceLayerExtensionsNames(strzilla::string_view);
    [[nodiscard]] std::vector<strzilla::string> GetAvailablePhysicalDeviceLayersNames();
    [[nodiscard]] std::vector<VkSurfaceFormatKHR> GetAvailablePhysicalDeviceSurfaceFormats();
    [[nodiscard]] std::vector<VkPresentModeKHR> GetAvailablePhysicalDeviceSurfacePresentationModes();
} // namespace RenderCore
