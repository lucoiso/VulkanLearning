// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <volk.h>

export module RenderCore.Utils.Helpers;

export import <cstdint>;
export import <vector>;
export import <mutex>;
export import <string>;
export import <string_view>;

namespace RenderCore
{
    export std::mutex g_CriticalEventMutex;

    export std::vector<char const*> GetGLFWExtensions();
    export VkExtent2D GetWindowExtent(GLFWwindow*, VkSurfaceCapabilitiesKHR const&);
    export std::vector<VkLayerProperties> GetAvailableInstanceLayers();
    export std::vector<std::string> GetAvailableInstanceLayersNames();
    export std::vector<VkExtensionProperties> GetAvailableInstanceExtensions();
    export std::vector<std::string> GetAvailableInstanceExtensionsNames();

#ifdef _DEBUG
    export void ListAvailableInstanceLayers();
    export void ListAvailableInstanceExtensions();
#endif

    export std::vector<VkExtensionProperties> GetAvailableLayerExtensions(std::string_view);
    export std::vector<std::string> GetAvailableLayerExtensionsNames(std::string_view);

#ifdef _DEBUG
    export void ListAvailableInstanceLayerExtensions(std::string_view);
#endif

    export std::array<VkVertexInputBindingDescription, 1U> GetBindingDescriptors();
    export std::array<VkVertexInputAttributeDescription, 4U> GetAttributeDescriptions();

    export template<typename T>
    constexpr bool CheckVulkanResult(T&& InputOperation)
    {
        if (VkResult const OperationResult = InputOperation;
            OperationResult != VK_SUCCESS)
        {
            return false;
        }

        return true;
    }
}// namespace RenderCore