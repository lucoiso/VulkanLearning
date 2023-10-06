// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <GLFW/glfw3.h>
#include <volk.h>

export module RenderCore.Management.DeviceManagement;

import <string_view>;
import <vector>;
import <cstdint>;

export namespace RenderCore
{
    void PickPhysicalDevice();
    void CreateLogicalDevice();

    void ReleaseDeviceResources();

    [[nodiscard]] std::vector<VkPhysicalDevice> GetAvailablePhysicalDevices();
    [[nodiscard]] std::vector<VkExtensionProperties> GetAvailablePhysicalDeviceExtensions();
    [[nodiscard]] std::vector<VkLayerProperties> GetAvailablePhysicalDeviceLayers();
    [[nodiscard]] std::vector<VkExtensionProperties> GetAvailablePhysicalDeviceLayerExtensions(std::string_view);
    [[nodiscard]] std::vector<std::string> GetAvailablePhysicalDeviceExtensionsNames();
    [[nodiscard]] std::vector<std::string> GetAvailablePhysicalDeviceLayerExtensionsNames(std::string_view);
    [[nodiscard]] std::vector<std::string> GetAvailablePhysicalDeviceLayersNames();
    [[nodiscard]] VkSurfaceCapabilitiesKHR GetAvailablePhysicalDeviceSurfaceCapabilities();
    [[nodiscard]] std::vector<VkSurfaceFormatKHR> GetAvailablePhysicalDeviceSurfaceFormats();
    [[nodiscard]] std::vector<VkPresentModeKHR> GetAvailablePhysicalDeviceSurfacePresentationModes();
    [[nodiscard]] VkDeviceSize GetMinUniformBufferOffsetAlignment();

    bool UpdateDeviceProperties(GLFWwindow* Window);
    [[nodiscard]] struct VulkanDeviceProperties& GetDeviceProperties();

    [[nodiscard]] VkDevice& GetLogicalDevice();
    [[nodiscard]] VkPhysicalDevice& GetPhysicalDevice();

    [[nodiscard]] std::pair<std::uint8_t, VkQueue>& GetGraphicsQueue();
    [[nodiscard]] std::pair<std::uint8_t, VkQueue>& GetPresentationQueue();
    [[nodiscard]] std::pair<std::uint8_t, VkQueue>& GetTransferQueue();

    [[nodiscard]] std::vector<std::uint8_t>& GetUniqueQueueFamilyIndices();
    [[nodiscard]] std::vector<std::uint32_t> GetUniqueQueueFamilyIndicesU32();

    [[nodiscard]] std::uint32_t GetMinImageCount();
}// namespace RenderCore