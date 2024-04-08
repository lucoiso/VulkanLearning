// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <unordered_map>
#include <vector>
#include <vma/vk_mem_alloc.h>

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
        TextureType   Type { TextureType::BaseColor };

        [[nodiscard]] bool IsValid() const;
        void               DestroyResources(VmaAllocator const &);
    };

    export struct BufferAllocation
    {
        VkBuffer      Buffer { VK_NULL_HANDLE };
        VmaAllocation Allocation { VK_NULL_HANDLE };
        void *        MappedData { nullptr };

        [[nodiscard]] bool IsValid() const;
        void               DestroyResources(VmaAllocator const &);
    };

    export enum class UniformType : std::uint32_t
    {
        Model,
        Material
    };

    export struct ObjectAllocationData
    {
        BufferAllocation                                        BufferAllocation {};
        std::vector<ImageAllocation>                            ImageAllocations {};
        std::unordered_map<UniformType, VkDescriptorBufferInfo> ModelDescriptors {};
        std::unordered_map<TextureType, VkDescriptorImageInfo>  TextureDescriptors {};

        [[nodiscard]] bool IsValid() const;
        void               DestroyResources(VmaAllocator const &);
    };
} // namespace RenderCore
