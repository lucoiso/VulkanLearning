// Copyright Notice: [...]

#pragma once

#include "RenderCoreModule.h"
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
        VulkanDeviceManager() = delete;
        VulkanDeviceManager(const VulkanDeviceManager&) = delete;
        VulkanDeviceManager& operator=(const VulkanDeviceManager&) = delete;

        VulkanDeviceManager(const VkInstance& Instance, const VkSurfaceKHR& Surface);
        ~VulkanDeviceManager();

        void PickPhysicalDevice(const VkPhysicalDevice& PreferredDevice = VK_NULL_HANDLE);
        void CreateLogicalDevice();

        void Shutdown();

        bool IsInitialized() const;
        [[nodiscard]] const VkDevice& GetLogicalDevice() const;
        [[nodiscard]] const VkPhysicalDevice& GetPhysicalDevice() const;
        [[nodiscard]] std::vector<const char*> GetAvailablePhysicalDeviceExtensions() const;
        [[nodiscard]] VkSurfaceCapabilitiesKHR GetAvailablePhysicalDeviceSurfaceCapabilities() const;
        [[nodiscard]] std::vector<VkSurfaceFormatKHR> GetAvailablePhysicalDeviceSurfaceFormats() const;
        [[nodiscard]] std::vector<VkPresentModeKHR> GetAvailablePhysicalDeviceSurfacePresentationModes() const;
        [[nodiscard]] std::vector<std::uint32_t> GetQueueFamilyIndices() const;
        [[nodiscard]] bool IsDeviceSuitable(const VkPhysicalDevice& Device) const;
        void GetSwapChainPreferredProperties(GLFWwindow* const Window, VkSurfaceFormatKHR& PreferredFormat, VkPresentModeKHR& PreferredMode, VkExtent2D& PreferredExtent, VkSurfaceCapabilitiesKHR& Capabilities);

    private:
        bool GetQueueFamilyIndices(std::optional<std::uint32_t>& GraphicsQueueFamilyIndex, std::optional<std::uint32_t>& PresentationQueueFamilyIndex);

        const VkInstance& m_Instance;
        const VkSurfaceKHR& m_Surface;

        VkPhysicalDevice m_PhysicalDevice;
        VkDevice m_Device;

        std::pair<std::uint32_t, VkQueue> m_GraphicsQueue;
        std::pair<std::uint32_t, VkQueue> m_PresentationQueue;
    };
}
