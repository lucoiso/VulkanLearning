// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Utils.Helpers;

export namespace RenderCore
{
    RENDERCOREMODULE_API void EmitFatalError(strzilla::string_view, std::source_location const &Location = std::source_location::current());

    RENDERCOREMODULE_API void CheckVulkanResult(VkResult InputOperation, std::source_location const &Location = std::source_location::current());

    template <typename T>
        requires std::is_invocable_v<T> && (std::is_same_v<std::invoke_result_t<T>, VkResult> || std::is_same_v<std::invoke_result_t<T>, VkResult &>)
    RENDERCOREMODULE_API constexpr void CheckVulkanResult(T &&InputOperation, std::source_location const &Location = std::source_location::current())
    {
        CheckVulkanResult(InputOperation(), Location);
    }

    RENDERCOREMODULE_API constexpr bool DepthHasStencil(VkFormat const &Format)
    {
        return Format >= VK_FORMAT_D16_UNORM_S8_UINT && Format <= VK_FORMAT_D32_SFLOAT_S8_UINT;
    }

    RENDERCOREMODULE_API [[nodiscard]] bool operator==(VkExtent2D, VkExtent2D);

    RENDERCOREMODULE_API [[nodiscard]] std::vector<VkLayerProperties> GetAvailableInstanceLayers();

    RENDERCOREMODULE_API [[nodiscard]] std::vector<strzilla::string> GetAvailableInstanceLayersNames();

    RENDERCOREMODULE_API [[nodiscard]] std::vector<VkExtensionProperties> GetAvailableInstanceExtensions();

    RENDERCOREMODULE_API [[nodiscard]] std::vector<strzilla::string> GetAvailableInstanceExtensionsNames();

    RENDERCOREMODULE_API [[nodiscard]] std::vector<VkExtensionProperties> GetAvailableInstanceLayerExtensions(strzilla::string_view);

    RENDERCOREMODULE_API [[nodiscard]] std::vector<strzilla::string> GetAvailableInstanceLayerExtensionsNames(strzilla::string_view);

    RENDERCOREMODULE_API [[nodiscard]] VkVertexInputBindingDescription GetBindingDescriptors(std::uint32_t);

    RENDERCOREMODULE_API [[nodiscard]] std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions(std::uint32_t,
                                                                                          std::vector<VkVertexInputAttributeDescription> const &);

    template <typename ItemType, typename ContainerType>
    RENDERCOREMODULE_API bool Contains(ContainerType const &Container, ItemType const &Item)
    {
        return std::ranges::find(Container, Item) != std::cend(Container);
    }

    template <typename Out, typename Opt, typename Avail>
    RENDERCOREMODULE_API void GetAvailableResources(char const * const Identifier, Out &Resource, Opt const &Optional, Avail const &Available)
    {
        std::for_each(std::execution::unseq,
                      std::cbegin(Resource),
                      std::cend(Resource),
                      [&Available, &Identifier](auto const &ResIter)
                      {
                          if (std::ranges::find(Available, ResIter) == std::cend(Available))
                          {
                              EmitFatalError(std::format("Required {} not available: {}", Identifier, ResIter));
                          }
                      });

        std::for_each(std::execution::unseq,
                      std::cbegin(Optional),
                      std::cend(Optional),
                      [&Available, &Resource](auto const &OptIter)
                      {
                          if (std::ranges::find(Available, OptIter) != std::cend(Available))
                          {
                              Resource.emplace_back(OptIter);
                          }
                      });
    }

    RENDERCOREMODULE_API void DispatchQueue(std::queue<std::function<void()>> &);

    template <typename ...Args>
    RENDERCOREMODULE_API VkDeviceSize GetAlignedSize(VkDeviceSize const InAlignment, std::size_t const Num = 1U)
    {
        VkDeviceSize Output = Num * sizeof...(Args);

        if (InAlignment > 0U)
        {
            Output = Output + InAlignment - 1U & ~(InAlignment - 1U);
        }

        return Output;
    }
} // namespace RenderCore
