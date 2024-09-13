// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Types.Allocation;

namespace RenderCore
{
    export struct RENDERCOREMODULE_API ImageAllocation
    {
        VkImage       Image { VK_NULL_HANDLE };
        VkImageView   View { VK_NULL_HANDLE };
        VmaAllocation Allocation { VK_NULL_HANDLE };
        VkExtent2D    Extent {};
        VkFormat      Format {};

        inline [[nodiscard]] bool IsValid() const
        {
            return Image != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE;
        }

        void DestroyResources(VmaAllocator const &);
    };

    export struct RENDERCOREMODULE_API BufferAllocation
    {
        VkBuffer      Buffer { VK_NULL_HANDLE };
        VkDeviceSize  Size { 0U };
        VmaAllocation Allocation { VK_NULL_HANDLE };
        void *        MappedData { nullptr };

        inline [[nodiscard]] bool IsValid() const
        {
            return Buffer != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE;
        }

        void                       DestroyResources(VmaAllocator const &);
        [[nodiscard]] VkDeviceSize GetAllocationSize(VmaAllocator const &) const;
    };

    export struct RENDERCOREMODULE_API DescriptorData
    {
        VkDescriptorSetLayout         SetLayout { VK_NULL_HANDLE };
        VkDeviceOrHostAddressConstKHR BufferDeviceAddress {};
        VkDeviceSize                  LayoutOffset { 0U };
        VkDeviceSize                  LayoutSize { 0U };
        BufferAllocation              Buffer {};

        inline[[nodiscard]] bool IsValid() const
        {
            return Buffer.IsValid() && SetLayout != VK_NULL_HANDLE;
        }

        void DestroyResources(VmaAllocator const &, bool);

        void SetDescriptorLayoutSize(VkDeviceSize const &);
    };
} // namespace RenderCore
