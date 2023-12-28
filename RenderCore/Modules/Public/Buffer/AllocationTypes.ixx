// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <vector>
#include <vk_mem_alloc.h>

export module RenderCore.Types.AllocationTypes;

import RenderCore.Utils.Constants;
import RenderCore.Types.Object;
import RenderCore.Types.Vertex;
import RenderCore.Types.MeshBufferData;
import RenderCore.Types.TextureType;
import RenderCore.Types.UniformBufferObject;

namespace RenderCore
{
    export struct ImageAllocation
    {
        VkImage Image{VK_NULL_HANDLE};
        VkImageView View{VK_NULL_HANDLE};
        VmaAllocation Allocation{VK_NULL_HANDLE};

        TextureType Type{TextureType::BaseColor};

        [[nodiscard]] bool IsValid() const;

        void DestroyResources(VmaAllocator const&);
    };

    export struct BufferAllocation
    {
        VkBuffer Buffer{VK_NULL_HANDLE};
        VmaAllocation Allocation{VK_NULL_HANDLE};
        void* MappedData{nullptr};

        [[nodiscard]] bool IsValid() const;

        void DestroyResources(VmaAllocator const&);
    };

    export struct ImageCreationData
    {
        ImageAllocation Allocation{};
        std::pair<VkBuffer, VmaAllocation> StagingBuffer{VK_NULL_HANDLE, VK_NULL_HANDLE};
        VkFormat Format{};
        VkExtent2D Extent{};
        TextureType Type{TextureType::BaseColor};
    };

    export struct CommandBufferSet
    {
        bool bVertexBuffer{false};
        VkCommandBuffer CommandBuffer{VK_NULL_HANDLE};
        VkDeviceSize AllocationSize{g_BufferMemoryAllocationSize};
        std::pair<VkBuffer, VmaAllocation> StagingBuffer{VK_NULL_HANDLE, VK_NULL_HANDLE};
    };

    export struct ObjectAllocation
    {
        std::vector<ImageAllocation> TextureImages{};
        BufferAllocation VertexBuffer{};
        BufferAllocation IndexBuffer{};
        BufferAllocation UniformBuffer{};
        std::uint32_t IndicesCount{0U};

        [[nodiscard]] bool IsValid() const;

        void DestroyResources(VmaAllocator const&);
    };

    export struct ObjectAllocationData
    {
        Object Object{0U, ""};
        ObjectAllocation Allocation{};
        std::vector<Vertex> Vertices{};
        std::vector<std::uint32_t> Indices{};
        std::vector<ImageCreationData> ImageCreationDatas{};
        std::vector<CommandBufferSet> CommandBufferSets{};
    };

    export struct BufferCopyTemporaryData
    {
        VkBuffer SourceBuffer{VK_NULL_HANDLE};
        VmaAllocation SourceAllocation{VK_NULL_HANDLE};

        VkBuffer DestinationBuffer{VK_NULL_HANDLE};
        VmaAllocation DestinationAllocation{VK_NULL_HANDLE};

        VkDeviceSize AllocationSize{0U};
    };

    export struct BufferCopyOperationData
    {
        BufferCopyTemporaryData VertexData{};
        BufferCopyTemporaryData IndexData{};
    };

    export struct MoveOperationData
    {
        VkImage Image{VK_NULL_HANDLE};
        VkFormat Format{VK_FORMAT_UNDEFINED};
    };

    export struct CopyOperationData
    {
        VkBuffer SourceBuffer{VK_NULL_HANDLE};
        VmaAllocation SourceAllocation{VK_NULL_HANDLE};

        VkImage DestinationImage{VK_NULL_HANDLE};
        VkFormat Format{VK_FORMAT_UNDEFINED};
        VkExtent2D Extent{0U, 0U};
    };
}// namespace RenderCore
