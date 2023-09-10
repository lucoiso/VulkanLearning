// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "RenderCoreModule.h"
#include "Types/DeviceProperties.h"
#include <vulkan/vulkan_core.h>
#include <vector>
#include <string>
#include <optional>

struct GLFWwindow;

namespace RenderCore
{
    class VulkanDeviceManager
    {
    public:
        VulkanDeviceManager() = delete;
        VulkanDeviceManager(const VulkanDeviceManager &) = delete;
        VulkanDeviceManager &operator=(const VulkanDeviceManager &) = delete;

        VulkanDeviceManager(const VkInstance &Instance, const VkSurfaceKHR &Surface);
        ~VulkanDeviceManager();

        void PickPhysicalDevice(const VkPhysicalDevice &PreferredDevice = VK_NULL_HANDLE);
        void CreateLogicalDevice();

        void Shutdown();

        bool IsInitialized() const;

        [[nodiscard]] const VkPhysicalDevice &GetPhysicalDevice() const;
        [[nodiscard]] const VkDevice &GetLogicalDevice() const;
        [[nodiscard]] const VkQueue &GetGraphicsQueue() const;
        [[nodiscard]] const VkQueue &GetPresentationQueue() const;
        [[nodiscard]] const VkQueue &GetTransferQueue() const;

        [[nodiscard]] std::vector<std::uint32_t> GetQueueFamilyIndices() const;
        [[nodiscard]] std::uint32_t GetGraphicsQueueFamilyIndex() const;
        [[nodiscard]] std::uint32_t GetPresentationQueueFamilyIndex() const;
        [[nodiscard]] std::uint32_t GetTransferQueueFamilyIndex() const;

        [[nodiscard]] std::vector<VkPhysicalDevice> GetAvailablePhysicalDevices() const;
        [[nodiscard]] std::vector<VkExtensionProperties> GetAvailablePhysicalDeviceExtensions() const;
        [[nodiscard]] std::vector<VkLayerProperties> GetAvailablePhysicalDeviceLayers() const;
        [[nodiscard]] std::vector<VkExtensionProperties> GetAvailablePhysicalDeviceLayerExtensions(const char *LayerName) const;
        [[nodiscard]] std::vector<const char *> GetAvailablePhysicalDeviceExtensionsNames() const;
        [[nodiscard]] std::vector<const char *> GetAvailablePhysicalDeviceLayersNames() const;
        [[nodiscard]] VkSurfaceCapabilitiesKHR GetAvailablePhysicalDeviceSurfaceCapabilities() const;
        [[nodiscard]] std::vector<VkSurfaceFormatKHR> GetAvailablePhysicalDeviceSurfaceFormats() const;
        [[nodiscard]] std::vector<VkPresentModeKHR> GetAvailablePhysicalDeviceSurfacePresentationModes() const;
        [[nodiscard]] VkDeviceSize GetMinUniformBufferOffsetAlignment() const;

        [[nodiscard]] bool IsPhysicalDeviceSuitable(const VkPhysicalDevice &Device) const;
        [[nodiscard]] DeviceProperties GetPreferredProperties(GLFWwindow *const Window);

    private:
        bool GetQueueFamilyIndices(std::optional<std::uint32_t> &GraphicsQueueFamilyIndex, std::optional<std::uint32_t> &PresentationQueueFamilyIndex, std::optional<std::uint32_t> &TransferQueueFamilyIndex);

#ifdef _DEBUG
        void ListAvailablePhysicalDevices() const;
        void ListAvailablePhysicalDeviceExtensions() const;
        void ListAvailablePhysicalDeviceLayers() const;
        void ListAvailablePhysicalDeviceLayerExtensions(const char *LayerName) const;
        void ListAvailablePhysicalDeviceSurfaceCapabilities() const;
        void ListAvailablePhysicalDeviceSurfaceFormats() const;
        void ListAvailablePhysicalDeviceSurfacePresentationModes() const;
#endif

        const VkInstance &m_Instance;
        const VkSurfaceKHR &m_Surface;

        VkPhysicalDevice m_PhysicalDevice;
        VkDevice m_Device;

        std::pair<std::uint32_t, VkQueue> m_GraphicsQueue;
        std::pair<std::uint32_t, VkQueue> m_PresentationQueue;
        std::pair<std::uint32_t, VkQueue> m_TransferQueue;
    };
}
