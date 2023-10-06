// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

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

export namespace RenderCore
{
    std::vector<char const*> GetGLFWExtensions();

    VkExtent2D GetWindowExtent(GLFWwindow*, VkSurfaceCapabilitiesKHR const&);

    std::vector<VkLayerProperties> GetAvailableInstanceLayers();

    std::vector<std::string> GetAvailableInstanceLayersNames();

    std::vector<VkExtensionProperties> GetAvailableInstanceExtensions();

    std::vector<std::string> GetAvailableInstanceExtensionsNames();

#ifdef _DEBUG
    void ListAvailableInstanceLayers();

    void ListAvailableInstanceExtensions();
#endif

    std::vector<VkExtensionProperties> GetAvailableLayerExtensions(std::string_view);

    std::vector<std::string> GetAvailableLayerExtensionsNames(std::string_view);

#ifdef _DEBUG
    void ListAvailableInstanceLayerExtensions(std::string_view);
#endif

    std::array<VkVertexInputBindingDescription, 1U> GetBindingDescriptors();

    std::array<VkVertexInputAttributeDescription, 3U> GetAttributeDescriptions();

    template<typename T1, typename T2>
    constexpr void AddFlags(T1& Lhs, T2 const Rhs)
    {
        Lhs = static_cast<T1>(static_cast<std::uint8_t>(Lhs) | static_cast<std::uint8_t>(Rhs));
    }

    template<typename T1, typename T2>
    constexpr void RemoveFlags(T1& Lhs, T2 const Rhs)
    {
        Lhs = static_cast<T1>(static_cast<std::uint8_t>(Lhs) & ~static_cast<std::uint8_t>(Rhs));
    }

    template<typename T1, typename T2>
    constexpr bool HasFlag(T1 const Lhs, T2 const Rhs)
    {
        return (Lhs & Rhs) == Rhs;
    }

    template<typename T1, typename T2>
    constexpr bool HasAnyFlag(T1 const Lhs, T2 const Rhs)
    {
        return static_cast<std::uint8_t>(Lhs & Rhs) != 0U;
    }

    template<typename T>
    constexpr bool HasAnyFlag(T const Lhs)
    {
        return static_cast<std::uint8_t>(Lhs) != 0U;
    }

    void InitializeSingleCommandQueue(VkCommandPool&, VkCommandBuffer&, std::uint8_t);

    void FinishSingleCommandQueue(VkQueue const&, VkCommandPool const&, VkCommandBuffer const&);

    struct UniformBufferObject GetUniformBufferObject();

    template<typename T>
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