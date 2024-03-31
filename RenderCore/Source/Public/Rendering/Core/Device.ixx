// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <Volk/volk.h>
#include <string>
#include <vector>

export module RenderCore.Runtime.Device;

import RenderCore.Types.SurfaceProperties;

export namespace RenderCore
{
    void InitializeDevice(VkSurfaceKHR const &);

    void ReleaseDeviceResources();

    [[nodiscard]] VkSurfaceCapabilitiesKHR GetSurfaceCapabilities(VkSurfaceKHR const &);

    [[nodiscard]] SurfaceProperties GetSurfaceProperties(GLFWwindow *Window, VkSurfaceKHR const &VulkanSurface);

    [[nodiscard]] VkDevice &GetLogicalDevice();

    [[nodiscard]] VkPhysicalDevice &GetPhysicalDevice();

    [[nodiscard]] std::pair<std::uint8_t, VkQueue> &GetGraphicsQueue();

    [[nodiscard]] std::pair<std::uint8_t, VkQueue> &GetPresentationQueue();

    [[nodiscard]] std::pair<std::uint8_t, VkQueue> &GetTransferQueue();

    [[nodiscard]] std::vector<std::uint32_t> GetUniqueQueueFamilyIndicesU32();

    [[nodiscard]] VkDeviceSize GetMinUniformBufferOffsetAlignment();

    [[nodiscard]] std::vector<VkPhysicalDevice> GetAvailablePhysicalDevices();

    [[nodiscard]] std::vector<VkExtensionProperties> GetAvailablePhysicalDeviceExtensions(VkPhysicalDevice const &);

    [[nodiscard]] std::vector<VkLayerProperties> GetAvailablePhysicalDeviceLayers(VkPhysicalDevice const &);

    [[nodiscard]] std::vector<VkExtensionProperties> GetAvailablePhysicalDeviceLayerExtensions(VkPhysicalDevice const &, std::string_view);

    [[nodiscard]] std::vector<std::string> GetAvailablePhysicalDeviceExtensionsNames(VkPhysicalDevice const &);

    [[nodiscard]] std::vector<std::string> GetAvailablePhysicalDeviceLayerExtensionsNames(VkPhysicalDevice const &, std::string_view);

    [[nodiscard]] std::vector<std::string> GetAvailablePhysicalDeviceLayersNames(VkPhysicalDevice const &);

    [[nodiscard]] std::vector<VkSurfaceFormatKHR> GetAvailablePhysicalDeviceSurfaceFormats(VkPhysicalDevice const &, VkSurfaceKHR const &);

    [[nodiscard]] std::vector<VkPresentModeKHR> GetAvailablePhysicalDeviceSurfacePresentationModes(VkPhysicalDevice const &, VkSurfaceKHR const &);
} // namespace RenderCore
