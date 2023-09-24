// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "Types/VulkanVertex.h"
#include "Types/TextureData.h"
#include <vector>
#include <string_view>
#include <unordered_map>
#include <volk.h>
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

        bool IsValid() const;
        void DestroyResources();
    };

    struct VulkanBufferAllocation
    {
        VkBuffer Buffer;
        VmaAllocation Allocation;

        bool IsValid() const;
        void DestroyResources();
    };

    struct VulkanObjectAllocation
    {
        VulkanImageAllocation TextureImage;
        VulkanBufferAllocation VertexBuffer;
        VulkanBufferAllocation IndexBuffer;
        std::uint32_t IndicesCount;

        bool IsValid() const;
        void DestroyResources();
    };

    class VulkanBufferManager
    {
    public:
        VulkanBufferManager(const VulkanBufferManager &) = delete;
        VulkanBufferManager &operator=(const VulkanBufferManager &) = delete;

        VulkanBufferManager();
        ~VulkanBufferManager();

        static VulkanBufferManager &Get();

        void CreateMemoryAllocator();
        void CreateSwapChain(const bool bRecreate);
        void CreateFrameBuffers(const VkRenderPass &RenderPass);
        void CreateDepthResources();

        std::uint64_t LoadObject(const std::string_view ModelPath, const std::string_view TexturePath);

        static VulkanImageAllocation AllocateTexture(const unsigned char *Data, const std::uint32_t Width, const std::uint32_t Height, const std::size_t AllocationSize);

        void DestroyResources(const bool bClearScene);
        void Shutdown();

        bool IsInitialized() const;
        [[nodiscard]] const VkSwapchainKHR &GetSwapChain() const;
        [[nodiscard]] const std::vector<VkImage> GetSwapChainImages() const;
        [[nodiscard]] const std::vector<VkFramebuffer> &GetFrameBuffers() const;
        [[nodiscard]] const VkBuffer GetVertexBuffer(const std::uint64_t ObjectID = 0u) const;
        [[nodiscard]] const VkBuffer GetIndexBuffer(const std::uint64_t ObjectID = 0u) const;
        [[nodiscard]] const std::uint32_t GetIndicesCount(const std::uint64_t ObjectID = 0u) const;
        const bool GetObjectTexture(const std::uint64_t ObjectID, VulkanTextureData &TextureData) const;

        static VmaAllocationInfo CreateBuffer(const VkDeviceSize &Size, const VkBufferUsageFlags Usage, const VkMemoryPropertyFlags Flags, VkBuffer &Buffer, VmaAllocation &Allocation);

        static void CopyBuffer(const VkBuffer &Source, const VkBuffer &Destination, const VkDeviceSize &Size, const VkQueue &Queue, const std::uint8_t QueueFamilyIndex);

        static void CreateImage(const VkFormat &ImageFormat, const VkExtent2D &Extent, const VkImageTiling &Tiling, const VkImageUsageFlags Usage, const VkMemoryPropertyFlags Flags, VkImage &Image, VmaAllocation &Allocation);

        static void CreateImageView(const VkImage &Image, const VkFormat &Format, const VkImageAspectFlags &AspectFlags, VkImageView &ImageView);

        static void CreateTextureImageView(VulkanImageAllocation &Allocation);

        static void CreateTextureSampler(VulkanImageAllocation &Allocation);

        static void CopyBufferToImage(const VkBuffer &Source, const VkImage &Destination, const VkExtent2D &Extent, const VkQueue &Queue, const std::uint32_t QueueFamilyIndex);

        static void MoveImageLayout(const VkImage &Image, const VkFormat &Format, const VkImageLayout &OldLayout, const VkImageLayout &NewLayout, const VkQueue &Queue, const std::uint8_t QueueFamilyIndex);

    private:
        void CreateVertexBuffers(VulkanObjectAllocation &Object, const std::vector<Vertex> &Vertices) const;

        void CreateIndexBuffers(VulkanObjectAllocation &Object, const std::vector<std::uint32_t> &Indices) const;

        void LoadTexture(VulkanObjectAllocation &Object, const std::string_view TexturePath) const;

        void CreateSwapChainImageViews(const VkFormat &ImageFormat);

    public:
        static VmaAllocator g_Allocator;

    private:
        static VulkanBufferManager g_Instance;

        VkSwapchainKHR m_SwapChain;
        VkSwapchainKHR m_OldSwapChain;
        std::vector<VulkanImageAllocation> m_SwapChainImages;
        VulkanImageAllocation m_DepthImage;
        std::vector<VkFramebuffer> m_FrameBuffers;
        std::unordered_map<std::uint64_t, VulkanObjectAllocation> m_Objects;
    };
}
