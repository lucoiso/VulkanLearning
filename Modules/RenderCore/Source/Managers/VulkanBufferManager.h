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
        VkImage       Image;
        VkImageView   View;
        VkSampler     Sampler;
        VmaAllocation Allocation;

        bool IsValid() const;
        void DestroyResources();
    };

    struct VulkanBufferAllocation
    {
        VkBuffer      Buffer;
        VmaAllocation Allocation;

        bool IsValid() const;
        void DestroyResources();
    };

    struct VulkanObjectAllocation
    {
        VulkanImageAllocation  TextureImage;
        VulkanBufferAllocation VertexBuffer;
        VulkanBufferAllocation IndexBuffer;
        std::uint32_t          IndicesCount;

        bool IsValid() const;
        void DestroyResources();
    };

    class VulkanBufferManager
    {
    public:
        VulkanBufferManager(const VulkanBufferManager&)            = delete;
        VulkanBufferManager& operator=(const VulkanBufferManager&) = delete;

        VulkanBufferManager();
        ~VulkanBufferManager();

        static VulkanBufferManager& Get();

        static void CreateMemoryAllocator();
        void        CreateSwapChain();
        void        CreateFrameBuffers();
        void        CreateDepthResources();

        std::uint64_t LoadObject(std::string_view ModelPath, std::string_view TexturePath);
        void          UnLoadObject(std::uint64_t ObjectID);

        static VulkanImageAllocation AllocateTexture(const unsigned char* Data, std::uint32_t Width, std::uint32_t Height, std::size_t AllocationSize);

        void DestroyResources(bool ClearScene);
        void Shutdown();

        static bool                                     IsInitialized();
        [[nodiscard]] const VkSwapchainKHR&             GetSwapChain() const;
        [[nodiscard]] const VkExtent2D&                 GetSwapChainExtent() const;
        [[nodiscard]] std::vector<VkImage>              GetSwapChainImages() const;
        [[nodiscard]] const std::vector<VkFramebuffer>& GetFrameBuffers() const;
        [[nodiscard]] VkBuffer                          GetVertexBuffer(std::uint64_t ObjectID = 0u) const;
        [[nodiscard]] VkBuffer                          GetIndexBuffer(std::uint64_t ObjectID = 0u) const;
        [[nodiscard]] std::uint32_t                     GetIndicesCount(std::uint64_t ObjectID = 0u) const;
        [[nodiscard]] std::vector<VulkanTextureData>    GetAllocatedTextures() const;

        static VmaAllocationInfo CreateBuffer(const VkDeviceSize& Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Flags, VkBuffer& Buffer, VmaAllocation& Allocation);

        static void CopyBuffer(const VkBuffer& Source, const VkBuffer& Destination, const VkDeviceSize& Size, const VkQueue& Queue, std::uint8_t QueueFamilyIndex);

        static void CreateImage(const VkFormat&       ImageFormat,
                                const VkExtent2D&     Extent,
                                const VkImageTiling&  Tiling,
                                VkImageUsageFlags     Usage,
                                VkMemoryPropertyFlags Flags,
                                VkImage&              Image,
                                VmaAllocation&        Allocation);

        static void CreateImageView(const VkImage& Image, const VkFormat& Format, const VkImageAspectFlags& AspectFlags, VkImageView& ImageView);

        static void CreateTextureImageView(VulkanImageAllocation& Allocation);

        static void CreateTextureSampler(VulkanImageAllocation& Allocation);

        static void CopyBufferToImage(const VkBuffer& Source, const VkImage& Destination, const VkExtent2D& Extent, const VkQueue& Queue, std::uint32_t QueueFamilyIndex);

        static void MoveImageLayout(const VkImage& Image, const VkFormat& Format, const VkImageLayout& OldLayout, const VkImageLayout& NewLayout, const VkQueue& Queue, std::uint8_t QueueFamilyIndex);

    private:
        static void CreateVertexBuffers(VulkanObjectAllocation& Object, const std::vector<Vertex>& Vertices);

        static void CreateIndexBuffers(VulkanObjectAllocation& Object, const std::vector<std::uint32_t>& Indices);

        static void LoadTexture(VulkanObjectAllocation& Object, std::string_view TexturePath);

        void CreateSwapChainImageViews(const VkFormat& ImageFormat);

    public:
        static VmaAllocator g_Allocator;

    private:
        static VulkanBufferManager g_Instance;

        VkSwapchainKHR                                            m_SwapChain;
        VkSwapchainKHR                                            m_OldSwapChain;
        VkExtent2D                                                m_SwapChainExtent;
        std::vector<VulkanImageAllocation>                        m_SwapChainImages;
        VulkanImageAllocation                                     m_DepthImage;
        std::vector<VkFramebuffer>                                m_FrameBuffers;
        std::unordered_map<std::uint64_t, VulkanObjectAllocation> m_Objects;
        std::atomic<std::uint64_t>                                m_ObjectIDCounter;
    };
}
