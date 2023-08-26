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
    class RENDERCOREMODULE_API VulkanConfigurator
    {
    public:
        VulkanConfigurator();

        VulkanConfigurator(const VulkanConfigurator&) = delete;
        VulkanConfigurator& operator=(const VulkanConfigurator&) = delete;

        ~VulkanConfigurator();

        void Initialize(GLFWwindow* const Window);
        void Shutdown();

        bool IsInitialized() const;
        [[nodiscard]] VkInstance GetInstance() const;
        [[nodiscard]] VkDevice GetLogicalDevice() const;
        [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const;
        [[nodiscard]] VkSurfaceCapabilitiesKHR GetPhysicalDeviceSurfaceCapabilities() const;
        [[nodiscard]] std::vector<VkSurfaceFormatKHR> GetPhysicalDeviceSurfaceFormats() const;
        [[nodiscard]] std::vector<VkPresentModeKHR> GetPhysicalDeviceSurfacePresentationModes() const;
        [[nodiscard]] std::vector<const char*> GetInstanceExtensions() const;
        [[nodiscard]] std::vector<const char*> GetPhysicalDeviceExtensions() const;
        [[nodiscard]] bool IsDeviceSuitable(const VkPhysicalDevice& Device) const;
        bool SupportsValidationLayer() const;

    private:
        void CreateInstance();
        void CreateSurface(GLFWwindow* const Window);
        void PickPhysicalDevice();
        void ChooseQueueFamilyIndices(std::optional<std::uint32_t>& GraphicsQueueFamilyIndex, std::optional<std::uint32_t>& PresentationQueueFamilyIndex);
        void CreateLogicalDevice(const std::uint32_t GraphicsQueueFamilyIndex, const std::uint32_t PresentationQueueFamilyIndex);
        void InitializeSwapChain();

        void CheckSupportedValidationLayers();
        void PopulateDebugInfo(VkDebugUtilsMessengerCreateInfoEXT& Info);
        void SetupDebugMessages();
        void ShutdownDebugMessages();

        VkInstance m_Instance;
        VkSurfaceKHR m_Surface;
        VkPhysicalDevice m_PhysicalDevice;
        VkDevice m_Device;
        VkQueue m_GraphicsQueue;
        VkQueue m_PresentationQueue;
        VkDebugUtilsMessengerEXT m_DebugMessenger;
        const std::vector<const char*> m_ValidationLayers;
        const std::vector<const char*> m_RequiredDeviceExtensions;
        bool m_SupportsValidationLayer;
    };
}
