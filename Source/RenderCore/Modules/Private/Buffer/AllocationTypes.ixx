// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <vk_mem_alloc.h>

export module RenderCore.Types.AllocationTypes;

export import <vector>;

export import RenderCore.Utils.Constants;
export import RenderCore.Types.MeshBufferData;
export import RenderCore.Types.TextureBufferData;
export import RenderCore.Types.UniformBufferObject;

namespace RenderCore
{
    export struct ImageAllocation
    {
        VkImage Image {VK_NULL_HANDLE};
        VkImageView View {VK_NULL_HANDLE};
        VkSampler Sampler {VK_NULL_HANDLE};
        VmaAllocation Allocation {VK_NULL_HANDLE};

        TextureType Type {TextureType::BaseColor};

        [[nodiscard]] bool IsValid() const;
        void DestroyResources(VmaAllocator const&);
    };

    export struct BufferAllocation
    {
        VkBuffer Buffer {VK_NULL_HANDLE};
        VmaAllocation Allocation {VK_NULL_HANDLE};
        void* MappedData {nullptr};

        [[nodiscard]] bool IsValid() const;
        void DestroyResources(VmaAllocator const&);
    };

    export struct ImageCreationData
    {
        ImageAllocation Allocation {};
        std::pair<VkBuffer, VmaAllocation> StagingBuffer {VK_NULL_HANDLE, VK_NULL_HANDLE};
        VkFormat Format {};
        VkExtent2D Extent {};
        TextureType Type {TextureType::BaseColor};
    };

    export struct CommandBufferSet
    {
        bool bVertexBuffer {false};
        VkCommandBuffer CommandBuffer {VK_NULL_HANDLE};
        VkDeviceSize AllocationSize {g_BufferMemoryAllocationSize};
        std::pair<VkBuffer, VmaAllocation> StagingBuffer {VK_NULL_HANDLE, VK_NULL_HANDLE};
    };

    export struct ObjectAllocation
    {
        std::vector<ImageAllocation> TextureImages {};
        BufferAllocation VertexBuffer {};
        BufferAllocation IndexBuffer {};
        BufferAllocation UniformBuffer {};
        std::uint32_t IndicesCount {0U};

        [[nodiscard]] bool IsValid() const;
        void DestroyResources(VmaAllocator const&);
    };
}// namespace RenderCore