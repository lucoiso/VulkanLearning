// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "RenderCoreModule.h"
#include "Types/DeviceProperties.h"
#include <QWindow>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <optional>

struct GLFWwindow;

namespace RenderCore
{
    class VulkanDeviceManager
    {
    public:
        VulkanDeviceManager(const VulkanDeviceManager &) = delete;
        VulkanDeviceManager &operator=(const VulkanDeviceManager &) = delete;

        VulkanDeviceManager();
        ~VulkanDeviceManager();

        void PickPhysicalDevice();
        void CreateLogicalDevice();

        void Shutdown();

        bool IsInitialized() const;

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
        [[nodiscard]] VulkanDeviceProperties GetPreferredProperties(QWindow *const Window);

    private:
        bool GetQueueFamilyIndices(std::optional<std::uint8_t> &GraphicsQueueFamilyIndex, std::optional<std::uint8_t> &PresentationQueueFamilyIndex, std::optional<std::uint8_t> &TransferQueueFamilyIndex);

#ifdef _DEBUG
        void ListAvailablePhysicalDevices() const;
        void ListAvailablePhysicalDeviceExtensions() const;
        void ListAvailablePhysicalDeviceLayers() const;
        void ListAvailablePhysicalDeviceLayerExtensions(const char *LayerName) const;
        void ListAvailablePhysicalDeviceSurfaceCapabilities() const;
        void ListAvailablePhysicalDeviceSurfaceFormats() const;
        void ListAvailablePhysicalDeviceSurfacePresentationModes() const;
#endif

        VkPhysicalDevice m_PhysicalDevice;
        VkDevice m_Device;

        std::pair<std::uint8_t, VkQueue> m_GraphicsQueue;
        std::pair<std::uint8_t, VkQueue> m_PresentationQueue;
        std::pair<std::uint8_t, VkQueue> m_TransferQueue;
    };
}
