// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <string>
#include <vector>
#include <GLFW/glfw3.h>
#include <Volk/volk.h>

export module RenderCore.Runtime.Device;

import RenderCore.Types.SurfaceProperties;

export namespace RenderCore
{
    void InitializeDevice(VkSurfaceKHR const &);

    void ReleaseDeviceResources();

    [[nodiscard]] VkSurfaceCapabilitiesKHR GetSurfaceCapabilities();

    [[nodiscard]] SurfaceProperties GetSurfaceProperties(GLFWwindow *);

    [[nodiscard]] VkDevice &GetLogicalDevice();

    [[nodiscard]] VkPhysicalDevice &GetPhysicalDevice();

    [[nodiscard]] std::pair<std::uint8_t, VkQueue> &GetGraphicsQueue();

    [[nodiscard]] std::pair<std::uint8_t, VkQueue> &GetComputeQueue();

    [[nodiscard]] std::pair<std::uint8_t, VkQueue> &GetPresentationQueue();

    [[nodiscard]] std::vector<std::uint32_t> GetUniqueQueueFamilyIndicesU32();

    [[nodiscard]] VkPhysicalDeviceProperties const &GetPhysicalDeviceProperties();

    [[nodiscard]] std::vector<VkPhysicalDevice> GetAvailablePhysicalDevices();

    [[nodiscard]] std::vector<VkExtensionProperties> GetAvailablePhysicalDeviceExtensions();

    [[nodiscard]] std::vector<VkLayerProperties> GetAvailablePhysicalDeviceLayers();

    [[nodiscard]] std::vector<VkExtensionProperties> GetAvailablePhysicalDeviceLayerExtensions(std::string_view);

    [[nodiscard]] std::vector<std::string> GetAvailablePhysicalDeviceExtensionsNames();

    [[nodiscard]] std::vector<std::string> GetAvailablePhysicalDeviceLayerExtensionsNames(std::string_view);

    [[nodiscard]] std::vector<std::string> GetAvailablePhysicalDeviceLayersNames();

    [[nodiscard]] std::vector<VkSurfaceFormatKHR> GetAvailablePhysicalDeviceSurfaceFormats();

    [[nodiscard]] std::vector<VkPresentModeKHR> GetAvailablePhysicalDeviceSurfacePresentationModes();
} // namespace RenderCore
