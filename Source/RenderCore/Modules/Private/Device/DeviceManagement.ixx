// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <volk.h>

export module RenderCore.Management.DeviceManagement;

export import <vector>;
export import <string>;
export import <string_view>;
export import <optional>;

export import RenderCore.Types.DeviceProperties;

namespace RenderCore
{
    export class DeviceManager
    {
        VkPhysicalDevice m_PhysicalDevice {VK_NULL_HANDLE};
        VkDevice m_Device {VK_NULL_HANDLE};
        DeviceProperties m_DeviceProperties {};
        std::pair<std::uint8_t, VkQueue> m_GraphicsQueue {};
        std::pair<std::uint8_t, VkQueue> m_PresentationQueue {};
        std::pair<std::uint8_t, VkQueue> m_TransferQueue {};
        std::vector<std::uint8_t> m_UniqueQueueFamilyIndices {};

        [[nodiscard]] bool GetQueueFamilyIndices(VkSurfaceKHR const& VulkanSurface,
                                                 std::optional<std::uint8_t>& GraphicsQueueFamilyIndex,
                                                 std::optional<std::uint8_t>& PresentationQueueFamilyIndex,
                                                 std::optional<std::uint8_t>& TransferQueueFamilyIndex) const;

    public:
        void PickPhysicalDevice(VkSurfaceKHR const&);
        void CreateLogicalDevice(VkSurfaceKHR const&);
        void ReleaseDeviceResources();

        bool UpdateDeviceProperties(GLFWwindow* Window, VkSurfaceKHR const& VulkanSurface);

        [[nodiscard]] DeviceProperties& GetDeviceProperties();
        [[nodiscard]] VkDevice& GetLogicalDevice();
        [[nodiscard]] VkPhysicalDevice& GetPhysicalDevice();
        [[nodiscard]] std::pair<std::uint8_t, VkQueue>& GetGraphicsQueue();
        [[nodiscard]] std::pair<std::uint8_t, VkQueue>& GetPresentationQueue();
        [[nodiscard]] std::pair<std::uint8_t, VkQueue>& GetTransferQueue();
        [[nodiscard]] std::vector<std::uint32_t> GetUniqueQueueFamilyIndicesU32() const;
        [[nodiscard]] std::uint32_t GetMinImageCount() const;
        [[nodiscard]] VkDeviceSize GetMinUniformBufferOffsetAlignment() const;
    };

    export [[nodiscard]] std::vector<VkPhysicalDevice> GetAvailablePhysicalDevices();
    export [[nodiscard]] std::vector<VkExtensionProperties> GetAvailablePhysicalDeviceExtensions(VkPhysicalDevice const&);
    export [[nodiscard]] std::vector<VkLayerProperties> GetAvailablePhysicalDeviceLayers(VkPhysicalDevice const&);
    export [[nodiscard]] std::vector<VkExtensionProperties> GetAvailablePhysicalDeviceLayerExtensions(VkPhysicalDevice const&, std::string_view const&);
    export [[nodiscard]] std::vector<std::string> GetAvailablePhysicalDeviceExtensionsNames(VkPhysicalDevice const&);
    export [[nodiscard]] std::vector<std::string> GetAvailablePhysicalDeviceLayerExtensionsNames(VkPhysicalDevice const&, std::string_view const&);
    export [[nodiscard]] std::vector<std::string> GetAvailablePhysicalDeviceLayersNames(VkPhysicalDevice const&);
    export [[nodiscard]] VkSurfaceCapabilitiesKHR GetAvailablePhysicalDeviceSurfaceCapabilities(VkPhysicalDevice const&, VkSurfaceKHR const&);
    export [[nodiscard]] std::vector<VkSurfaceFormatKHR> GetAvailablePhysicalDeviceSurfaceFormats(VkPhysicalDevice const&, VkSurfaceKHR const&);
    export [[nodiscard]] std::vector<VkPresentModeKHR> GetAvailablePhysicalDeviceSurfacePresentationModes(VkPhysicalDevice const&, VkSurfaceKHR const&);
}// namespace RenderCore