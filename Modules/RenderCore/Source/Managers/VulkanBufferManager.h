// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "Types/VulkanVertex.h"
#include "Types/TextureData.h"
#include <vector>
#include <string_view>
#include <unordered_map>
#include <atomic>
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
        void CreateSwapChain();
        void CreateFrameBuffers();
        void CreateDepthResources();

        std::uint64_t LoadObject(const std::string_view ModelPath, const std::string_view TexturePath);
        void UnLoadObject(const std::uint64_t ObjectID);

        VulkanImageAllocation AllocateTexture(const unsigned char *Data, const std::uint32_t Width, const std::uint32_t Height, const std::size_t AllocationSize) const;

        void DestroyResources(const bool bClearScene);
        void Shutdown();

        bool IsInitialized() const;
        [[nodiscard]] const VkSwapchainKHR &GetSwapChain() const;
        [[nodiscard]] const VkExtent2D &GetSwapChainExtent() const;
        [[nodiscard]] const std::vector<VkImage> GetSwapChainImages() const;
        [[nodiscard]] const std::vector<VkFramebuffer> &GetFrameBuffers() const;
        [[nodiscard]] const VkBuffer GetVertexBuffer(const std::uint64_t ObjectID = 0u) const;
        [[nodiscard]] const VkBuffer GetIndexBuffer(const std::uint64_t ObjectID = 0u) const;
        [[nodiscard]] const std::uint32_t GetIndicesCount(const std::uint64_t ObjectID = 0u) const;
        [[nodiscard]] std::vector<VulkanTextureData> GetAllocatedTextures() const;

        VmaAllocationInfo CreateBuffer(const VkDeviceSize &Size, const VkBufferUsageFlags Usage, const VkMemoryPropertyFlags Flags, VkBuffer &Buffer, VmaAllocation &Allocation) const;

        void CopyBuffer(const VkBuffer &Source, const VkBuffer &Destination, const VkDeviceSize &Size, const VkQueue &Queue, const std::uint8_t QueueFamilyIndex) const;

        void CreateImage(const VkFormat &ImageFormat, const VkExtent2D &Extent, const VkImageTiling &Tiling, const VkImageUsageFlags Usage, const VkMemoryPropertyFlags Flags, VkImage &Image, VmaAllocation &Allocation) const;

        void CreateImageView(const VkImage &Image, const VkFormat &Format, const VkImageAspectFlags &AspectFlags, VkImageView &ImageView) const;

        void CreateTextureImageView(VulkanImageAllocation &Allocation) const;

        void CreateTextureSampler(VulkanImageAllocation &Allocation) const;

        void CopyBufferToImage(const VkBuffer &Source, const VkImage &Destination, const VkExtent2D &Extent, const VkQueue &Queue, const std::uint32_t QueueFamilyIndex) const;

        void MoveImageLayout(const VkImage &Image, const VkFormat &Format, const VkImageLayout &OldLayout, const VkImageLayout &NewLayout, const VkQueue &Queue, const std::uint8_t QueueFamilyIndex) const;

    private:
        void CreateVertexBuffers(VulkanObjectAllocation &Object, const std::vector<Vertex> &Vertices) const;

        void CreateIndexBuffers(VulkanObjectAllocation &Object, const std::vector<std::uint32_t> &Indices) const;

        void LoadTexture(VulkanObjectAllocation &Object, const std::string_view TexturePath) const;

        void CreateSwapChainImageViews(const VkFormat &ImageFormat);

    public:
        static VmaAllocator s_Allocator;

    private:
        static VulkanBufferManager g_Instance;

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
