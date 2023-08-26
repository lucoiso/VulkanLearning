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
        VkInstance GetInstance() const;
        VkDevice GetDevice() const;
        VkPhysicalDevice GetPhysicalDevice() const;
        std::vector<const char*> GetInstanceExtensions() const;
        std::vector<const char*> GetDeviceExtensions() const;
        bool IsDeviceSuitable(const VkPhysicalDevice& Device) const;
        bool SupportsValidationLayer() const;

    private:
        void CreateInstance();
        void CreateSurface(GLFWwindow* const Window);
        void PickPhysicalDevice();
        void ChooseQueueFamilyIndices(std::optional<std::uint32_t>& graphicsQueueFamilyIndex, std::optional<std::uint32_t>& presentQueueFamilyIndex);
        void CreateDevice(const std::uint32_t GraphicsQueueFamilyIndex, const std::uint32_t PresentQueueFamilyIndex);
        void CheckSupportedValidationLayers();
        void PopulateDebugInfo(VkDebugUtilsMessengerCreateInfoEXT& Info);
        void SetupDebugMessages();
        void ShutdownDebugMessages();

        VkInstance m_Instance;
        VkSurfaceKHR m_Surface;
        VkPhysicalDevice m_PhysicalDevice;
        VkDevice m_Device;
        VkQueue m_GraphicsQueue;
        VkQueue m_PresentQueue;
        VkDebugUtilsMessengerEXT m_DebugMessenger;
        const std::vector<const char*> m_ValidationLayers;
        const std::vector<const char*> m_RequiredDeviceExtensions;
        bool m_SupportsValidationLayer;
    };
}
