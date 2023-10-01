// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

#pragma once

#include "Types/VulkanVertex.h"
#include "Types/TextureData.h"
#include <vector>
#include <string_view>
#include <unordered_map>
#include <atomic>
#include <vk_mem_alloc.h>

struct GLFWwindow;

namespace RenderCore
{
    struct VulkanImageAllocation
    {
        VkImage Image;
        VkImageView View;
        VkSampler Sampler;
        VmaAllocation Allocation;

        [[nodiscard]] bool IsValid() const;
        void DestroyResources();
    };

    struct VulkanBufferAllocation
    {
        VkBuffer Buffer;
        VmaAllocation Allocation;

        [[nodiscard]] bool IsValid() const;
        void DestroyResources();
    };

    struct VulkanObjectAllocation
    {
        VulkanImageAllocation TextureImage;
        VulkanBufferAllocation VertexBuffer;
        VulkanBufferAllocation IndexBuffer;
        std::uint32_t IndicesCount;

        [[nodiscard]] bool IsValid() const;
        void DestroyResources();
    };

    class VulkanBufferManager final // NOLINT(cppcoreguidelines-special-member-functions)
    {
    public:
        VulkanBufferManager();

        VulkanBufferManager(VulkanBufferManager const&)            = delete;
        VulkanBufferManager& operator=(VulkanBufferManager const&) = delete;

        ~VulkanBufferManager();

        static VulkanBufferManager& Get();

        void CreateMemoryAllocator();
        void CreateSwapChain();
        void CreateFrameBuffers();
        void CreateDepthResources();

        std::uint64_t LoadObject(std::string_view ModelPath, std::string_view TexturePath);
        void UnLoadObject(std::uint64_t ObjectID);

        VulkanImageAllocation AllocateTexture(unsigned char const* Data, std::uint32_t Width, std::uint32_t Height, std::size_t AllocationSize) const;

        void DestroyResources(bool ClearScene);
        void Shutdown();

        [[nodiscard]] bool IsInitialized() const;
        [[nodiscard]] VmaAllocator const& GetAllocator() const;
        [[nodiscard]] VkSwapchainKHR const& GetSwapChain() const;
        [[nodiscard]] VkExtent2D const& GetSwapChainExtent() const;
        [[nodiscard]] std::vector<VkImage> GetSwapChainImages() const;
        [[nodiscard]] std::vector<VkFramebuffer> const& GetFrameBuffers() const;
        [[nodiscard]] VkBuffer GetVertexBuffer(std::uint64_t ObjectID = 0U) const;
        [[nodiscard]] VkBuffer GetIndexBuffer(std::uint64_t ObjectID = 0U) const;
        [[nodiscard]] std::uint32_t GetIndicesCount(std::uint64_t ObjectID = 0U) const;
        [[nodiscard]] std::vector<VulkanTextureData> GetAllocatedTextures() const;

        VmaAllocationInfo CreateBuffer(VkDeviceSize const& Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Flags, VkBuffer& Buffer, VmaAllocation& Allocation) const;

        static void CopyBuffer(VkBuffer const& Source, VkBuffer const& Destination, VkDeviceSize const& Size, VkQueue const& Queue, std::uint8_t QueueFamilyIndex);

        void CreateImage(VkFormat const& ImageFormat,
                         VkExtent2D const& Extent,
                         VkImageTiling const& Tiling,
                         VkImageUsageFlags Usage,
                         VkMemoryPropertyFlags Flags,
                         VkImage& Image,
                         VmaAllocation& Allocation) const;

        static void CreateImageView(VkImage const& Image, VkFormat const& Format, VkImageAspectFlags const& AspectFlags, VkImageView& ImageView);

        void CreateTextureImageView(VulkanImageAllocation& Allocation) const;

        void CreateTextureSampler(VulkanImageAllocation& Allocation) const;

        static void CopyBufferToImage(VkBuffer const& Source, VkImage const& Destination, VkExtent2D const& Extent, VkQueue const& Queue, std::uint32_t QueueFamilyIndex);

        static void MoveImageLayout(VkImage const& Image, VkFormat const& Format, VkImageLayout const& OldLayout, VkImageLayout const& NewLayout, VkQueue const& Queue, std::uint8_t QueueFamilyIndex);

    private:
        void CreateVertexBuffers(VulkanObjectAllocation& Object, std::vector<Vertex> const& Vertices) const;

        void CreateIndexBuffers(VulkanObjectAllocation& Object, std::vector<std::uint32_t> const& Indices) const;

        void LoadTexture(VulkanObjectAllocation& Object, std::string_view TexturePath) const;

        void CreateSwapChainImageViews(VkFormat const& ImageFormat);

        VmaAllocator m_Allocator{};
        VkSwapchainKHR m_SwapChain;
        VkSwapchainKHR m_OldSwapChain;
        VkExtent2D m_SwapChainExtent;
        std::vector<VulkanImageAllocation> m_SwapChainImages;
        VulkanImageAllocation m_DepthImage;
        std::vector<VkFramebuffer> m_FrameBuffers;
        std::unordered_map<std::uint64_t, VulkanObjectAllocation> m_Objects;
        std::atomic<std::uint64_t> m_ObjectIDCounter;
    };
}
