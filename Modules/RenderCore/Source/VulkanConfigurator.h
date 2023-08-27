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
    class VulkanConfigurator
    {
    public:
        VulkanConfigurator();

        VulkanConfigurator(const VulkanConfigurator&) = delete;
        VulkanConfigurator& operator=(const VulkanConfigurator&) = delete;

        ~VulkanConfigurator();

        void CreateInstance();
        void UpdateSupportsValidationLayer();
        void CreateSurface(GLFWwindow* const Window);
        void PickPhysicalDevice(const VkPhysicalDevice& PreferredDevice = VK_NULL_HANDLE);
        void CreateLogicalDevice();
        void InitializeSwapChain(GLFWwindow* const Window);
        void InitializeSwapChain(const VkSurfaceFormatKHR& PreferredFormat, const VkPresentModeKHR& PreferredMode, const VkExtent2D& PreferredExtent, const VkSurfaceCapabilitiesKHR& Capabilities);

        void Shutdown();

        bool IsInitialized() const;
        [[nodiscard]] VkInstance GetInstance() const;
        [[nodiscard]] VkDevice GetLogicalDevice() const;
        [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const;
        [[nodiscard]] VkSurfaceCapabilitiesKHR GetAvailablePhysicalDeviceSurfaceCapabilities() const;
        [[nodiscard]] std::vector<VkSurfaceFormatKHR> GetAvailablePhysicalDeviceSurfaceFormats() const;
        [[nodiscard]] std::vector<VkPresentModeKHR> GetAvailablePhysicalDeviceSurfacePresentationModes() const;
        [[nodiscard]] std::vector<const char*> GetAvailableInstanceExtensions() const;
        [[nodiscard]] std::vector<const char*> GetAvailablePhysicalDeviceExtensions() const;
        [[nodiscard]] std::vector<VkLayerProperties> GetAvailableValidationLayers() const;
        [[nodiscard]] bool IsDeviceSuitable(const VkPhysicalDevice& Device) const;
        bool SupportsValidationLayer() const;

    private:
        bool GetQueueFamilyIndices(std::optional<std::uint32_t>& GraphicsQueueFamilyIndex, std::optional<std::uint32_t>& PresentationQueueFamilyIndex);
        void PopulateDebugInfo(VkDebugUtilsMessengerCreateInfoEXT& Info);
        void SetupDebugMessages();
        void ShutdownDebugMessages();

        void ResetSwapChain(const bool bDestroyImages = true);
        void CreateSwapChainImageViews(const VkFormat& ImageFormat);

        VkInstance m_Instance;
        VkSurfaceKHR m_Surface;
        VkPhysicalDevice m_PhysicalDevice;
        VkDevice m_Device;
        VkSwapchainKHR m_SwapChain;
        std::vector<VkImage> m_SwapChainImages;
        std::vector<VkImageView> m_SwapChainImageViews;
        std::pair<std::uint32_t, VkQueue> m_GraphicsQueue;
        std::pair<std::uint32_t, VkQueue> m_PresentationQueue;
        VkDebugUtilsMessengerEXT m_DebugMessenger;
        const std::vector<const char*> m_ValidationLayers;
        const std::vector<const char*> m_RequiredDeviceExtensions;
        bool m_SupportsValidationLayer;
    };
}
