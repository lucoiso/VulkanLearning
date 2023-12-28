// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <string>
#include <unordered_map>
#include <vector>
#include <volk.h>
#include <GLFW/glfw3.h>

export module RenderCore.Utils.Helpers;

namespace RenderCore
{
    export [[nodiscard]] VkExtent2D GetWindowExtent(GLFWwindow*, VkSurfaceCapabilitiesKHR const&);

    export [[nodiscard]] std::vector<std::string> GetGLFWExtensions();

    export [[nodiscard]] std::vector<VkLayerProperties> GetAvailableInstanceLayers();

    export [[nodiscard]] std::vector<std::string> GetAvailableInstanceLayersNames();

    export [[nodiscard]] std::vector<VkExtensionProperties> GetAvailableInstanceExtensions();

    export [[nodiscard]] std::vector<std::string> GetAvailableInstanceExtensionsNames();

    export [[nodiscard]] std::vector<VkExtensionProperties> GetAvailableLayerExtensions(std::string_view const&);

    export [[nodiscard]] std::vector<std::string> GetAvailableLayerExtensionsNames(std::string_view const&);

    export [[nodiscard]] std::array<VkVertexInputBindingDescription, 1U> GetBindingDescriptors();

    export [[nodiscard]] std::array<VkVertexInputAttributeDescription, 4U> GetAttributeDescriptions();

    export template<typename T>
        requires std::is_same_v<T, VkResult> || std::is_same_v<T, VkResult&>
    constexpr bool CheckVulkanResult(T&& InputOperation)
    {
        return InputOperation == VK_SUCCESS;
    }

    export template<typename T>
        requires std::is_invocable_v<T> && (std::is_same_v<std::invoke_result_t<T>, VkResult> || std::is_same_v<std::invoke_result_t<T>, VkResult&>)
    constexpr bool CheckVulkanResult(T&& InputOperation)
    {
        return CheckVulkanResult(InputOperation());
    }
}// namespace RenderCore
