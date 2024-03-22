// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <string>
#include <vector>
#include <Volk/volk.h>
#include <GLFW/glfw3.h>

export module RenderCore.Management.DeviceManagement;

import RenderCore.Types.SurfaceProperties;

namespace RenderCore
{
    export void InitializeDevice(VkSurfaceKHR const &);

    export void ReleaseDeviceResources();

    export [[nodiscard]] VkSurfaceCapabilitiesKHR GetSurfaceCapabilities(VkSurfaceKHR const &);

    export [[nodiscard]] SurfaceProperties GetSurfaceProperties(GLFWwindow *Window, VkSurfaceKHR const &VulkanSurface);

    export [[nodiscard]] VkDevice &GetLogicalDevice();

    export [[nodiscard]] VkPhysicalDevice &GetPhysicalDevice();

    export [[nodiscard]] std::pair<std::uint8_t, VkQueue> &GetGraphicsQueue();

    export [[nodiscard]] std::pair<std::uint8_t, VkQueue> &GetPresentationQueue();

    export [[nodiscard]] std::pair<std::uint8_t, VkQueue> &GetTransferQueue();

    export [[nodiscard]] std::vector<std::uint32_t> GetUniqueQueueFamilyIndicesU32();

    export [[nodiscard]] VkDeviceSize GetMinUniformBufferOffsetAlignment();

    export [[nodiscard]] std::vector<VkPhysicalDevice> GetAvailablePhysicalDevices();

    export [[nodiscard]] std::vector<VkExtensionProperties> GetAvailablePhysicalDeviceExtensions(VkPhysicalDevice const &);

    export [[nodiscard]] std::vector<VkLayerProperties> GetAvailablePhysicalDeviceLayers(VkPhysicalDevice const &);

    export [[nodiscard]] std::vector<VkExtensionProperties> GetAvailablePhysicalDeviceLayerExtensions(VkPhysicalDevice const &, std::string_view);

    export [[nodiscard]] std::vector<std::string> GetAvailablePhysicalDeviceExtensionsNames(VkPhysicalDevice const &);

    export [[nodiscard]] std::vector<std::string> GetAvailablePhysicalDeviceLayerExtensionsNames(VkPhysicalDevice const &, std::string_view);

    export [[nodiscard]] std::vector<std::string> GetAvailablePhysicalDeviceLayersNames(VkPhysicalDevice const &);

    export [[nodiscard]] std::vector<VkSurfaceFormatKHR> GetAvailablePhysicalDeviceSurfaceFormats(VkPhysicalDevice const &, VkSurfaceKHR const &);

    export [[nodiscard]] std::vector<VkPresentModeKHR> GetAvailablePhysicalDeviceSurfacePresentationModes(VkPhysicalDevice const &, VkSurfaceKHR const &);
} // namespace RenderCore
