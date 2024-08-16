// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Types.Allocation;

import RenderCore.Utils.Constants;
import RenderCore.Types.Vertex;
import RenderCore.Types.Material;
import RenderCore.Types.UniformBufferObject;

namespace RenderCore
{
    export struct ImageAllocation
    {
        VkImage       Image { VK_NULL_HANDLE };
        VkImageView   View { VK_NULL_HANDLE };
        VmaAllocation Allocation { VK_NULL_HANDLE };
        VkExtent2D    Extent {};
        VkFormat      Format {};

        [[nodiscard]] bool IsValid() const;
        void               DestroyResources(VmaAllocator const &);
    };

    export struct BufferAllocation
    {
        VkBuffer      Buffer { VK_NULL_HANDLE };
        VkDeviceSize  Size { 0U };
        VmaAllocation Allocation { VK_NULL_HANDLE };
        void *        MappedData { nullptr };

        [[nodiscard]] bool         IsValid() const;
        void                       DestroyResources(VmaAllocator const &);
        [[nodiscard]] VkDeviceSize GetAllocationSize(VmaAllocator const &) const;
    };

    export struct DescriptorData
    {
        VkDescriptorSetLayout         SetLayout { VK_NULL_HANDLE };
        VkDeviceOrHostAddressConstKHR BufferDeviceAddress {};
        VkDeviceSize                  LayoutOffset { 0U };
        VkDeviceSize                  LayoutSize { 0U };
        BufferAllocation              Buffer {};

        [[nodiscard]] bool IsValid() const;
        void               DestroyResources(VmaAllocator const &, bool);

        void SetDescriptorLayoutSize(VkDeviceSize const &);
    };
} // namespace RenderCore
