// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <vk_mem_alloc.h>
#include <volk.h>

export module RenderCoreBufferManager;

import <atomic>;
import <string_view>;
import <unordered_map>;
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

    class BufferManager final
    {
        VmaAllocator m_Allocator {};
        VkSwapchainKHR m_SwapChain;
        VkSwapchainKHR m_OldSwapChain;
        VkExtent2D m_SwapChainExtent;
        std::vector<ImageAllocation> m_SwapChainImages;
        ImageAllocation m_DepthImage;
        ImageAllocation m_ImGuiFontImage;
        std::vector<VkFramebuffer> m_FrameBuffers;
        std::unordered_map<std::uint64_t, ObjectAllocation> m_Objects;
        std::atomic<std::uint64_t> m_ObjectIDCounter;

    public:
        BufferManager();

        BufferManager(BufferManager const&)            = delete;
        BufferManager& operator=(BufferManager const&) = delete;

        ~BufferManager();

        static BufferManager& Get();

        void CreateMemoryAllocator();
        void CreateSwapChain();
        void CreateFrameBuffers();
        void CreateDepthResources();
        void LoadImGuiFonts();

        std::uint64_t LoadObject(std::string_view ModelPath, std::string_view TexturePath);
        void UnLoadObject(std::uint64_t ObjectID);

        ImageAllocation AllocateTexture(unsigned char const* Data, std::uint32_t Width, std::uint32_t Height, std::size_t AllocationSize) const;

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
        [[nodiscard]] std::vector<struct VulkanTextureData> GetAllocatedTextures() const;
        [[nodiscard]] struct VulkanTextureData GetAllocatedImGuiFontTexture() const;

        VmaAllocationInfo CreateBuffer(VkDeviceSize const& Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Flags, VkBuffer& Buffer, VmaAllocation& Allocation) const;
        void CreateVertexBuffer(ObjectAllocation& Object, VkDeviceSize const& AllocationSize, std::vector<struct Vertex> const& Vertices) const;
        void CreateIndexBuffer(ObjectAllocation& Object, VkDeviceSize const& AllocationSize, std::vector<std::uint32_t> const& Indices) const;

        static void CopyBuffer(VkBuffer const& Source, VkBuffer const& Destination, VkDeviceSize const& Size, VkQueue const& Queue, std::uint8_t QueueFamilyIndex);

        void CreateImage(VkFormat const& ImageFormat,
                         VkExtent2D const& Extent,
                         VkImageTiling const& Tiling,
                         VkImageUsageFlags Usage,
                         VkMemoryPropertyFlags Flags,
                         VkImage& Image,
                         VmaAllocation& Allocation) const;

        static void CreateImageView(VkImage const& Image, VkFormat const& Format, VkImageAspectFlags const& AspectFlags, VkImageView& ImageView);

        static void CreateTextureImageView(ImageAllocation& Allocation);

        void CreateTextureSampler(ImageAllocation& Allocation) const;

        static void CopyBufferToImage(VkBuffer const& Source, VkImage const& Destination, VkExtent2D const& Extent, VkQueue const& Queue, std::uint32_t QueueFamilyIndex);

        static void MoveImageLayout(VkImage const& Image, VkFormat const& Format, VkImageLayout const& OldLayout, VkImageLayout const& NewLayout, VkQueue const& Queue, std::uint8_t QueueFamilyIndex);

    private:
        void CreateVertexBuffers(ObjectAllocation& Object, std::vector<Vertex> const& Vertices) const;

        void CreateIndexBuffers(ObjectAllocation& Object, std::vector<std::uint32_t> const& Indices) const;

        void LoadTexture(ObjectAllocation& Object, std::string_view TexturePath) const;

        void CreateSwapChainImageViews(VkFormat const& ImageFormat);
    };
}// namespace RenderCore