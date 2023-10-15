// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <volk.h>

export module RenderCore.Utils.Helpers;

import <algorithm>;
import <array>;
import <chrono>;
import <concepts>;
import <cstdint>;
import <string>;
import <span>;

namespace RenderCore
{
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

    export template<typename T1, typename T2>
    constexpr void AddFlags(T1& Lhs, T2 const Rhs)
    {
        Lhs = static_cast<T1>(static_cast<std::uint8_t>(Lhs) | static_cast<std::uint8_t>(Rhs));
    }

    export template<typename T1, typename T2>
    constexpr void RemoveFlags(T1& Lhs, T2 const Rhs)
    {
        Lhs = static_cast<T1>(static_cast<std::uint8_t>(Lhs) & ~static_cast<std::uint8_t>(Rhs));
    }

    export template<typename T1, typename T2>
    constexpr bool HasFlag(T1 const Lhs, T2 const Rhs)
    {
        return (Lhs & Rhs) == Rhs;
    }

    export template<typename T1, typename T2>
    constexpr bool HasAnyFlag(T1 const Lhs, T2 const Rhs)
    {
        return static_cast<std::uint8_t>(Lhs & Rhs) != 0U;
    }

    export template<typename T>
    constexpr bool HasAnyFlag(T const Lhs)
    {
        return static_cast<std::uint8_t>(Lhs) != 0U;
    }

    export void InitializeSingleCommandQueue(VkCommandPool&, VkCommandBuffer&, std::uint8_t);
    export void FinishSingleCommandQueue(VkQueue const&, VkCommandPool const&, VkCommandBuffer const&);

    export void UpdateUniformBuffers();

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