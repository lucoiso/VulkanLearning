// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "Utils/VulkanEnumConverter.h"
#include "Types/VulkanVertex.h"
#include "Types/VulkanUniformBufferObject.h"
#include <vector>
#include <string>
#include <string_view>
#include <stdexcept>
#include <volk.h>

struct GLFWwindow;

namespace RenderCore
{
#define RENDERCORE_CHECK_VULKAN_RESULT(INPUT_OPERATION)                                                                   \
    if (const VkResult OperationResult = INPUT_OPERATION; OperationResult != VK_SUCCESS)                                  \
    {                                                                                                                     \
        throw std::runtime_error("Vulkan operation failed with result: " + std::string(ResultToString(OperationResult))); \
    }

    class RenderCoreHelpers
    {
    public:
        static std::vector<const char *> GetGLFWExtensions();

        static VkExtent2D GetWindowExtent(GLFWwindow *const Window, const VkSurfaceCapabilitiesKHR &Capabilities);

        static std::vector<VkLayerProperties> GetAvailableInstanceLayers();

        static std::vector<std::string> GetAvailableInstanceLayersNames();

        static std::vector<VkExtensionProperties> GetAvailableInstanceExtensions();

        static std::vector<std::string> GetAvailableInstanceExtensionsNames();

    #ifdef _DEBUG
        static void ListAvailableInstanceLayers();

        static void ListAvailableInstanceExtensions();
    #endif

        static std::vector<VkExtensionProperties> GetAvailableLayerExtensions(const std::string_view LayerName);

        static std::vector<std::string> GetAvailableLayerExtensionsNames(const std::string_view LayerName);

    #ifdef _DEBUG
        static void ListAvailableInstanceLayerExtensions(const std::string_view LayerName);
    #endif

    static constexpr std::array<VkVertexInputBindingDescription, 1> GetBindingDescriptors()
    {
        return {
            VkVertexInputBindingDescription{
                .binding = 0u,
                .stride = sizeof(Vertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX}};
    }

    static constexpr std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
    {
        return {
            VkVertexInputAttributeDescription{
                .location = 0u,
                .binding = 0u,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, Position)},
            VkVertexInputAttributeDescription{
                .location = 1u,
                .binding = 0u,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, Color)},
            VkVertexInputAttributeDescription{
                .location = 2u,
                .binding = 0u,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex, TextureCoordinate)}};
    }

        template <typename T1, typename T2>
        static constexpr void AddFlags(T1 &Lhs, const T2 Rhs)
        {
            Lhs = static_cast<T1>(static_cast<std::uint8_t>(Lhs) | static_cast<std::uint8_t>(Rhs));
        }

        template <typename T1, typename T2>
        static constexpr void RemoveFlags(T1 &Lhs, const T2 Rhs)
        {
            Lhs = static_cast<T1>(static_cast<std::uint8_t>(Lhs) & ~static_cast<std::uint8_t>(Rhs));
        }

        template <typename T1, typename T2>
        static constexpr bool HasFlag(const T1 Lhs, const T2 Rhs)
        {
            return (Lhs & Rhs) == Rhs;
        }

        template <typename T1, typename T2>
        static constexpr bool HasAnyFlag(const T1 Lhs, const T2 Rhs)
        {
            return static_cast<std::uint8_t>(Lhs & Rhs) != 0u;
        }

        template <typename T>
        static constexpr bool HasAnyFlag(const T Lhs)
        {
            return static_cast<std::uint8_t>(Lhs) != 0u;
        }

        static void InitializeSingleCommandQueue(VkCommandPool &CommandPool, VkCommandBuffer &CommandBuffer, const std::uint8_t QueueFamilyIndex);

        static void FinishSingleCommandQueue(const VkQueue &Queue, const VkCommandPool &CommandPool, const VkCommandBuffer &CommandBuffer);

        static UniformBufferObject GetUniformBufferObject();
    };
}