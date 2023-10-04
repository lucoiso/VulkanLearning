// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <GLFW/glfw3.h>
#include <volk.h>

export module RenderCore.Managers.DeviceManager;

import <optional>;
import <string_view>;
import <vector>;
import <cstdint>;
import <unordered_map>;

export namespace RenderCore
{
    class DeviceManager final
    {
        VkPhysicalDevice m_PhysicalDevice;
        VkDevice m_Device;
        std::pair<std::uint8_t, VkQueue> m_GraphicsQueue;
        std::pair<std::uint8_t, VkQueue> m_PresentationQueue;
        std::pair<std::uint8_t, VkQueue> m_TransferQueue;
        std::vector<std::uint8_t> m_UniqueQueueFamilyIndices;

    public:
        DeviceManager();

        DeviceManager(DeviceManager const&)            = delete;
        DeviceManager& operator=(DeviceManager const&) = delete;

        ~DeviceManager();

        static DeviceManager& Get();

        void PickPhysicalDevice();
        void CreateLogicalDevice();

        void Shutdown();

        [[nodiscard]] bool IsInitialized() const;

        [[nodiscard]] static std::vector<VkPhysicalDevice> GetAvailablePhysicalDevices();
        [[nodiscard]] std::vector<VkExtensionProperties> GetAvailablePhysicalDeviceExtensions() const;
        [[nodiscard]] std::vector<VkLayerProperties> GetAvailablePhysicalDeviceLayers() const;
        [[nodiscard]] std::vector<VkExtensionProperties> GetAvailablePhysicalDeviceLayerExtensions(std::string_view LayerName) const;
        [[nodiscard]] std::vector<std::string> GetAvailablePhysicalDeviceExtensionsNames() const;
        [[nodiscard]] std::vector<std::string> GetAvailablePhysicalDeviceLayerExtensionsNames(std::string_view LayerName) const;
        [[nodiscard]] std::vector<std::string> GetAvailablePhysicalDeviceLayersNames() const;
        [[nodiscard]] VkSurfaceCapabilitiesKHR GetAvailablePhysicalDeviceSurfaceCapabilities() const;
        [[nodiscard]] std::vector<VkSurfaceFormatKHR> GetAvailablePhysicalDeviceSurfaceFormats() const;
        [[nodiscard]] std::vector<VkPresentModeKHR> GetAvailablePhysicalDeviceSurfacePresentationModes() const;
        [[nodiscard]] VkDeviceSize GetMinUniformBufferOffsetAlignment() const;

        [[nodiscard]] static bool IsPhysicalDeviceSuitable(VkPhysicalDevice const& Device);

        bool UpdateDeviceProperties(GLFWwindow* Window) const;
        [[nodiscard]] static struct VulkanDeviceProperties& GetDeviceProperties();

        [[nodiscard]] VkDevice& GetLogicalDevice();
        [[nodiscard]] VkPhysicalDevice& GetPhysicalDevice();

        [[nodiscard]] std::pair<std::uint8_t, VkQueue>& GetGraphicsQueue();
        [[nodiscard]] std::pair<std::uint8_t, VkQueue>& GetPresentationQueue();
        [[nodiscard]] std::pair<std::uint8_t, VkQueue>& GetTransferQueue();

        [[nodiscard]] std::vector<std::uint8_t>& GetUniqueQueueFamilyIndices();
        [[nodiscard]] std::vector<std::uint32_t> GetUniqueQueueFamilyIndicesU32();

        [[nodiscard]] std::uint32_t GetMinImageCount() const;

    private:
        bool GetQueueFamilyIndices(std::optional<std::uint8_t>& GraphicsQueueFamilyIndex,
                                   std::optional<std::uint8_t>& PresentationQueueFamilyIndex,
                                   std::optional<std::uint8_t>& TransferQueueFamilyIndex) const;

#ifdef _DEBUG
        static void ListAvailablePhysicalDevices();
        void ListAvailablePhysicalDeviceExtensions() const;
        void ListAvailablePhysicalDeviceLayers() const;
        void ListAvailablePhysicalDeviceLayerExtensions(std::string_view LayerName) const;
        void ListAvailablePhysicalDeviceSurfaceCapabilities() const;
        void ListAvailablePhysicalDeviceSurfaceFormats() const;
        void ListAvailablePhysicalDeviceSurfacePresentationModes() const;
#endif
    };
}// namespace RenderCore