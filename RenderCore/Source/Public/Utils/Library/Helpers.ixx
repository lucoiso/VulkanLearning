// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <format>
#include <stdexcept>
#include <string>
#include <vector>
#include <volk.h>
#include <algorithm>

export module RenderCore.Utils.Helpers;

import RuntimeInfo.Manager;

namespace RenderCore
{
    export [[nodiscard]] VkExtent2D GetWindowExtent(GLFWwindow*, VkSurfaceCapabilitiesKHR const&);

    export [[nodiscard]] std::vector<std::string> GetGLFWExtensions();

    export [[nodiscard]] std::vector<VkLayerProperties> GetAvailableInstanceLayers();

    export [[nodiscard]] std::vector<std::string> GetAvailableInstanceLayersNames();

    export [[nodiscard]] std::vector<VkExtensionProperties> GetAvailableInstanceExtensions();

    export [[nodiscard]] std::vector<std::string> GetAvailableInstanceExtensionsNames();

    export [[nodiscard]] std::vector<VkExtensionProperties> GetAvailableInstanceLayerExtensions(std::string_view);

    export [[nodiscard]] std::vector<std::string> GetAvailableInstanceLayerExtensionsNames(std::string_view);

    export [[nodiscard]] std::array<VkVertexInputBindingDescription, 1U> GetBindingDescriptors();

    export [[nodiscard]] std::array<VkVertexInputAttributeDescription, 4U> GetAttributeDescriptions();

    export template<typename ItemType, typename ContainerType> constexpr bool Contains(ContainerType const& Container, ItemType const& Item)
    {
        return std::ranges::find(Container, Item) != std::cend(Container);
    }

    export template<typename Out, typename Opt, typename Avail> constexpr void GetAvailableResources(std::string_view const Identifier, Out& Resource, Opt const& Optional, Avail const& Available)
    {
        std::ranges::for_each(Resource, [&Available, Identifier](auto const& ResIter) {
            if (std::ranges::find(Available, ResIter) == std::cend(Available))
            {
                throw std::runtime_error(std::format("Required {} not available: {}", Identifier, ResIter));
            }
        });

        std::ranges::for_each(Optional, [&Available, &Resource](auto const& OptIter) {
            if (std::ranges::find(Available, OptIter) != std::cend(Available))
            {
                Resource.emplace_back(OptIter);
            }
        });
    }

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
