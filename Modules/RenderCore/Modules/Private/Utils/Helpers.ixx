// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <volk.h>

export module RenderCore.Utils.Helpers;

export import <cstdint>;
export import <vector>;
export import <string>;
export import <string_view>;

namespace RenderCore
{
    export VkExtent2D GetWindowExtent(GLFWwindow*, VkSurfaceCapabilitiesKHR const&);
    export std::vector<char const*> GetGLFWExtensions();
    export std::vector<VkLayerProperties> GetAvailableInstanceLayers();
    export std::vector<std::string> GetAvailableInstanceLayersNames();
    export std::vector<VkExtensionProperties> GetAvailableInstanceExtensions();
    export std::vector<std::string> GetAvailableInstanceExtensionsNames();

#ifdef _DEBUG
    export void ListAvailableInstanceLayers();
    export void ListAvailableInstanceExtensions();
#endif

    export std::vector<VkExtensionProperties> GetAvailableLayerExtensions(std::string_view const&);
    export std::vector<std::string> GetAvailableLayerExtensionsNames(std::string_view const&);

#ifdef _DEBUG
    export void ListAvailableInstanceLayerExtensions(std::string_view const&);
#endif

    export std::array<VkVertexInputBindingDescription, 1U> GetBindingDescriptors();
    export std::array<VkVertexInputAttributeDescription, 4U> GetAttributeDescriptions();

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