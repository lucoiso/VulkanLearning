// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <GLFW/glfw3.h>
#include <array>
#include <string>
#include <vector>
#include <volk.h>

export module RenderCore.Utils.RenderCoreHelpers;

namespace RenderCore
{
    class RenderCoreHelpers
    {
    public:
        static std::vector<char const*> GetGLFWExtensions();

        static VkExtent2D GetWindowExtent(GLFWwindow* Window, VkSurfaceCapabilitiesKHR const& Capabilities);

        static std::vector<VkLayerProperties> GetAvailableInstanceLayers();

        static std::vector<std::string> GetAvailableInstanceLayersNames();

        static std::vector<VkExtensionProperties> GetAvailableInstanceExtensions();

        static std::vector<std::string> GetAvailableInstanceExtensionsNames();

#ifdef _DEBUG
        static void ListAvailableInstanceLayers();

        static void ListAvailableInstanceExtensions();
#endif

        static std::vector<VkExtensionProperties> GetAvailableLayerExtensions(std::string_view LayerName);

        static std::vector<std::string> GetAvailableLayerExtensionsNames(std::string_view LayerName);

#ifdef _DEBUG
        static void ListAvailableInstanceLayerExtensions(std::string_view LayerName);
#endif

        static std::array<VkVertexInputBindingDescription, 1U> GetBindingDescriptors();

        static std::array<VkVertexInputAttributeDescription, 3U> GetAttributeDescriptions();

        template<typename T1, typename T2>
        static constexpr void AddFlags(T1& Lhs, T2 const Rhs)
        {
            Lhs = static_cast<T1>(static_cast<std::uint8_t>(Lhs) | static_cast<std::uint8_t>(Rhs));
        }

        template<typename T1, typename T2>
        static constexpr void RemoveFlags(T1& Lhs, T2 const Rhs)
        {
            Lhs = static_cast<T1>(static_cast<std::uint8_t>(Lhs) & ~static_cast<std::uint8_t>(Rhs));
        }

        template<typename T1, typename T2>
        static constexpr bool HasFlag(T1 const Lhs, T2 const Rhs)
        {
            return (Lhs & Rhs) == Rhs;
        }

        template<typename T1, typename T2>
        static constexpr bool HasAnyFlag(T1 const Lhs, T2 const Rhs)
        {
            return static_cast<std::uint8_t>(Lhs & Rhs) != 0U;
        }

        template<typename T>
        static constexpr bool HasAnyFlag(T const Lhs)
        {
            return static_cast<std::uint8_t>(Lhs) != 0U;
        }

        static void InitializeSingleCommandQueue(VkCommandPool& CommandPool, VkCommandBuffer& CommandBuffer, std::uint8_t QueueFamilyIndex);

        static void FinishSingleCommandQueue(VkQueue const& Queue, VkCommandPool const& CommandPool, VkCommandBuffer const& CommandBuffer);

        static struct UniformBufferObject GetUniformBufferObject();

        template<typename T>
        constexpr static bool CheckVulkanResult(T&& InputOperation)
        {
            if (VkResult const OperationResult = InputOperation;
                OperationResult != VK_SUCCESS)
            {
                return false;
            }

            return true;
        }
    };
}// namespace RenderCore