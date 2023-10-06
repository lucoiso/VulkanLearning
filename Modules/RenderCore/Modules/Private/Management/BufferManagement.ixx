// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <vk_mem_alloc.h>
#include <volk.h>

export module RenderCore.Management.BufferManagement;

import <string_view>;
import <vector>;

export namespace RenderCore
{
    struct ImageAllocation
    {
        VkImage Image;
        VkImageView View;
        VkSampler Sampler;
        VmaAllocation Allocation;

        [[nodiscard]] bool IsValid() const;
        void DestroyResources();
    };

    struct BufferAllocation
    {
        VkBuffer Buffer;
        VmaAllocation Allocation;

        [[nodiscard]] bool IsValid() const;
        void DestroyResources();
    };

    struct ObjectAllocation
    {
        ImageAllocation TextureImage;
        BufferAllocation VertexBuffer;
        BufferAllocation IndexBuffer;
        std::uint32_t IndicesCount;

        [[nodiscard]] bool IsValid() const;
        void DestroyResources();
    };

    void CreateMemoryAllocator();
    void CreateSwapChain();
    void CreateFrameBuffers();
    void CreateDepthResources();

    std::uint64_t LoadObject(std::string_view, std::string_view);
    void UnLoadObject(std::uint64_t);

    void DestroyBufferResources(bool);
    void ReleaseBufferResources();

    [[nodiscard]] VmaAllocator const& GetAllocator();
    [[nodiscard]] VkSwapchainKHR const& GetSwapChain();
    [[nodiscard]] VkExtent2D const& GetSwapChainExtent();
    [[nodiscard]] std::vector<VkFramebuffer> const& GetFrameBuffers();
    [[nodiscard]] VkBuffer GetVertexBuffer(std::uint64_t);
    [[nodiscard]] VkBuffer GetIndexBuffer(std::uint64_t);
    [[nodiscard]] std::uint32_t GetIndicesCount(std::uint64_t);
    [[nodiscard]] std::vector<struct VulkanTextureData> GetAllocatedTextures();
}// namespace RenderCore