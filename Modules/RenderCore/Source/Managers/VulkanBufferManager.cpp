// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "Managers/VulkanBufferManager.h"
#include "Managers/VulkanRenderSubsystem.h"
#include "Utils/RenderCoreHelpers.h"
#include <boost/log/trivial.hpp>
#include <chrono>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>

#ifndef VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#endif
#include "vk_mem_alloc.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include <stb_image.h>

using namespace RenderCore;

class VulkanBufferManager::Impl
{
    struct VulkanImageAllocation
    {
        VkImage Image;
        VkImageView View;
        VkSampler Sampler;
        VmaAllocation Allocation;

        bool IsValid() const noexcept
        {
            return Image != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE;
        }

        void DestroyResources(const VmaAllocator& Allocator) noexcept
        {
            if (Image != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE)
            {
                vmaDestroyImage(Allocator, Image, Allocation);
                Image = VK_NULL_HANDLE;
                Allocation = VK_NULL_HANDLE;
            }

            const VkDevice &VulkanLogicalDevice = VulkanRenderSubsystem::Get()->GetDevice();

            if (View != VK_NULL_HANDLE)
            {
                vkDestroyImageView(VulkanLogicalDevice, View, nullptr);
                View = VK_NULL_HANDLE;

                if (Image != VK_NULL_HANDLE)
                {
                    Image = VK_NULL_HANDLE;
                }
            }

            if (Sampler != VK_NULL_HANDLE)
            {
                vkDestroySampler(VulkanLogicalDevice, Sampler, nullptr);
                Sampler = VK_NULL_HANDLE;
            }
        }
    };

    struct VulkanBufferAllocation
    {
        VkBuffer Buffer;
        VmaAllocation Allocation;
        void* Data;

        bool IsValid() const noexcept
        {
            return Buffer != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE && Data != nullptr;
        }

        void DestroyResources(const VmaAllocator& Allocator) noexcept
        {
            if (Data)
            {
                vmaUnmapMemory(Allocator, Allocation);
                Data = nullptr;
            }

            if (Buffer != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE)
            {
                vmaDestroyBuffer(Allocator, Buffer, Allocation);
                Allocation = VK_NULL_HANDLE;
                Buffer = VK_NULL_HANDLE;
            }
        }
    };

    struct VulkanObjectAllocation
    {
        std::vector<VulkanImageAllocation> TextureImages;
        std::vector<VulkanBufferAllocation> VertexBuffers;
        std::vector<VulkanBufferAllocation> IndexBuffers;
        std::vector<VulkanBufferAllocation> UniformBuffers;
        std::vector<Vertex> Vertices;
        std::vector<std::uint32_t> Indices;

        bool IsValid() const
        {
            return !TextureImages.empty()
                && !VertexBuffers.empty()
                && !IndexBuffers.empty()
                && !UniformBuffers.empty()
                && !Vertices.empty()
                && !Indices.empty();
        }

        void DestroyResources(const bool bClearAll, const VmaAllocator& Allocator)
        {
            for (VulkanBufferAllocation& BufferIter : VertexBuffers)
            {
                BufferIter.DestroyResources(Allocator);
            }
            VertexBuffers.clear();

            for (VulkanBufferAllocation& BufferIter : IndexBuffers)
            {
                BufferIter.DestroyResources(Allocator);
            }
            IndexBuffers.clear();

            if (bClearAll)
            {
                for (VulkanImageAllocation& ImageIter : TextureImages)
                {
                    ImageIter.DestroyResources(Allocator);
                }
                TextureImages.clear();

                for (VulkanBufferAllocation& BufferIter : UniformBuffers)
                {
                    BufferIter.DestroyResources(Allocator);
                }
                UniformBuffers.clear();

                Vertices.clear();
                Indices.clear();
            }
        }
    };

public:
    Impl(const VulkanBufferManager &) = delete;
    Impl &operator=(const VulkanBufferManager &) = delete;

    Impl()
        : m_Allocator(VK_NULL_HANDLE)
        , m_SwapChain(VK_NULL_HANDLE)
        , m_SwapChainImages({})
        , m_DepthImage()
        , m_FrameBuffers({})
        , m_Objects({})
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan buffer manager";
    }

    ~Impl()
    {
        if (!IsInitialized())
        {
            return;
        }

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing vulkan buffer manager";
        Shutdown();
    }
    
    void CreateMemoryAllocator()
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan memory allocator";

        const VmaAllocatorCreateInfo AllocatorInfo{
            .flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT,
            .physicalDevice = VulkanRenderSubsystem::Get()->GetPhysicalDevice(),
            .device = VulkanRenderSubsystem::Get()->GetDevice(),
            .preferredLargeHeapBlockSize = 0u /*Default: 256 MiB*/,
            .pAllocationCallbacks = nullptr,
            .pDeviceMemoryCallbacks = nullptr,
            .pHeapSizeLimit = nullptr,
            .pVulkanFunctions = nullptr,
            .instance = VulkanRenderSubsystem::Get()->GetInstance(),
            .vulkanApiVersion = VK_API_VERSION_1_0,
            .pTypeExternalMemoryHandleTypes = nullptr
        };

        RENDERCORE_CHECK_VULKAN_RESULT(vmaCreateAllocator(&AllocatorInfo, &m_Allocator));
    }

    void CreateSwapChain(const bool bRecreate)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan swap chain";

        if (bRecreate)
        {
            DestroyResources(false);
        }

        const VulkanDeviceProperties &Properties = VulkanRenderSubsystem::Get()->GetDeviceProperties();

        const std::vector<std::uint32_t> QueueFamilyIndices = VulkanRenderSubsystem::Get()->GetQueueFamilyIndices_u32();
        const std::uint32_t QueueFamilyIndicesCount = static_cast<std::uint32_t>(QueueFamilyIndices.size());
        
        const VkSwapchainCreateInfoKHR SwapChainCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = VulkanRenderSubsystem::Get()->GetSurface(),
            .minImageCount = VulkanRenderSubsystem::Get()->GetMinImageCount(),
            .imageFormat = Properties.Format.format,
            .imageColorSpace = Properties.Format.colorSpace,
            .imageExtent = Properties.Extent,
            .imageArrayLayers = 1u,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = QueueFamilyIndicesCount > 1u ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = QueueFamilyIndicesCount,
            .pQueueFamilyIndices = QueueFamilyIndices.data(),
            .preTransform = Properties.Capabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = Properties.Mode,
            .clipped = VK_TRUE,
        };
        
        const VkDevice &VulkanLogicalDevice = VulkanRenderSubsystem::Get()->GetDevice();

        RENDERCORE_CHECK_VULKAN_RESULT(vkCreateSwapchainKHR(VulkanLogicalDevice, &SwapChainCreateInfo, nullptr, &m_SwapChain));

        std::uint32_t Count = 0u;
        RENDERCORE_CHECK_VULKAN_RESULT(vkGetSwapchainImagesKHR(VulkanLogicalDevice, m_SwapChain, &Count, nullptr));

        std::vector<VkImage> SwapChainImages(Count, VK_NULL_HANDLE);
        RENDERCORE_CHECK_VULKAN_RESULT(vkGetSwapchainImagesKHR(VulkanLogicalDevice, m_SwapChain, &Count, SwapChainImages.data()));
        
        m_SwapChainImages.resize(Count);
        for (std::uint32_t Iterator = 0u; Iterator < Count; ++Iterator)
        {
            m_SwapChainImages[Iterator].Image = SwapChainImages[Iterator];
        }

        CreateSwapChainImageViews(Properties.Format.format);

        CreateVertexBuffers();
        CreateIndexBuffers();

        if (!bRecreate)
        {
            CreateUniformBuffers();
        }
    }

    void CreateFrameBuffers(const VkRenderPass &RenderPass)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan frame buffers";

        if (RenderPass == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan render pass is invalid.");
        }

        const VkDevice &VulkanLogicalDevice = VulkanRenderSubsystem::Get()->GetDevice();
        const VulkanDeviceProperties &Properties = VulkanRenderSubsystem::Get()->GetDeviceProperties();

        m_FrameBuffers.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);

        for (std::uint32_t Iterator = 0u; Iterator < static_cast<std::uint32_t>(m_FrameBuffers.size()); ++Iterator)
        {
            const std::array<VkImageView, 2u> Attachments{
                m_SwapChainImages[Iterator].View,
                m_DepthImage.View};

            const VkFramebufferCreateInfo FrameBufferCreateInfo{
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = RenderPass,
                .attachmentCount = static_cast<std::uint32_t>(Attachments.size()),
                .pAttachments = Attachments.data(),
                .width = Properties.Extent.width,
                .height = Properties.Extent.height,
                .layers = 1u};

            RENDERCORE_CHECK_VULKAN_RESULT(vkCreateFramebuffer(VulkanLogicalDevice, &FrameBufferCreateInfo, nullptr, &m_FrameBuffers[Iterator]));
        }
    }

    void CreateVertexBuffers()
    {
        if (m_Objects.empty())
        {
            return;
        }

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan vertex buffers";

        if (m_Allocator == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan memory allocator is invalid.");
        }

        const VkQueue &TransferQueue = VulkanRenderSubsystem::Get()->GetQueueFromType(VulkanQueueType::Transfer);
        const std::uint8_t TransferQueueFamilyIndex = VulkanRenderSubsystem::Get()->GetQueueFamilyIndexFromType(VulkanQueueType::Transfer);

        constexpr VkBufferUsageFlags SourceUsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        constexpr VkBufferUsageFlags DestinationUsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        for (auto &[ID, Object] : m_Objects)
        {
            Object.VertexBuffers.resize(m_SwapChainImages.size());

            const VkDeviceSize BufferSize = Object.Vertices.size() * sizeof(Vertex);

            VkPhysicalDeviceMemoryProperties MemoryProperties;
            vkGetPhysicalDeviceMemoryProperties(m_Allocator->GetPhysicalDevice(), &MemoryProperties);

            for (std::uint32_t Iterator = 0u; Iterator < static_cast<std::uint32_t>(Object.VertexBuffers.size()); ++Iterator)
            {
                VkBuffer StagingBuffer;
                VmaAllocation StagingBufferMemory;
                CreateBuffer(MemoryProperties, BufferSize, SourceUsageFlags, SourceMemoryPropertyFlags, StagingBuffer, StagingBufferMemory);

                void *Data;
                vmaMapMemory(m_Allocator, StagingBufferMemory, &Data);
                std::memcpy(Data, Object.Vertices.data(), static_cast<std::size_t>(BufferSize));
                vmaUnmapMemory(m_Allocator, StagingBufferMemory);

                CreateBuffer(MemoryProperties, BufferSize, DestinationUsageFlags, DestinationMemoryPropertyFlags, Object.VertexBuffers[Iterator].Buffer, Object.VertexBuffers[Iterator].Allocation);
                CopyBuffer(StagingBuffer, Object.VertexBuffers[Iterator].Buffer, BufferSize, TransferQueue, TransferQueueFamilyIndex);

                vmaDestroyBuffer(m_Allocator, StagingBuffer, StagingBufferMemory);
            }
        }
    }

    void CreateIndexBuffers()
    {
        if (m_Objects.empty())
        {
            return;
        }

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan index buffers";

        if (m_Allocator == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan memory allocator is invalid.");
        }

        const VkQueue &TransferQueue = VulkanRenderSubsystem::Get()->GetQueueFromType(VulkanQueueType::Transfer);
        const std::uint8_t TransferQueueFamilyIndex = VulkanRenderSubsystem::Get()->GetQueueFamilyIndexFromType(VulkanQueueType::Transfer);

        constexpr VkBufferUsageFlags SourceUsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        constexpr VkBufferUsageFlags DestinationUsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        for (auto &[ID, Object] : m_Objects)
        {
            Object.IndexBuffers.resize(m_SwapChainImages.size());

            const VkDeviceSize BufferSize = Object.Indices.size() * sizeof(std::uint32_t);

            VkPhysicalDeviceMemoryProperties MemoryProperties;
            vkGetPhysicalDeviceMemoryProperties(m_Allocator->GetPhysicalDevice(), &MemoryProperties);

            for (std::uint32_t Iterator = 0u; Iterator < static_cast<std::uint32_t>(Object.IndexBuffers.size()); ++Iterator)
            {
                VkBuffer StagingBuffer;
                VmaAllocation StagingBufferMemory;
                CreateBuffer(MemoryProperties, BufferSize, SourceUsageFlags, SourceMemoryPropertyFlags, StagingBuffer, StagingBufferMemory);

                void *Data;
                vmaMapMemory(m_Allocator, StagingBufferMemory, &Data);
                std::memcpy(Data, Object.Indices.data(), static_cast<std::size_t>(BufferSize));
                vmaUnmapMemory(m_Allocator, StagingBufferMemory);

                CreateBuffer(MemoryProperties, BufferSize, DestinationUsageFlags, DestinationMemoryPropertyFlags, Object.IndexBuffers[Iterator].Buffer, Object.IndexBuffers[Iterator].Allocation);
                CopyBuffer(StagingBuffer, Object.IndexBuffers[Iterator].Buffer, BufferSize, TransferQueue, TransferQueueFamilyIndex);

                vmaDestroyBuffer(m_Allocator, StagingBuffer, StagingBufferMemory);
            }
        }
    }

    void CreateUniformBuffers()
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan index buffers";

        if (m_Allocator == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan memory allocator is invalid.");
        }

        constexpr VkBufferUsageFlags UsageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        constexpr VkMemoryPropertyFlags MemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        for (auto &[ID, Object] : m_Objects)
        {
            Object.UniformBuffers.resize(g_MaxFramesInFlight);

            const VkDeviceSize BufferSize = sizeof(UniformBufferObject);

            VkPhysicalDeviceMemoryProperties MemoryProperties;
            vkGetPhysicalDeviceMemoryProperties(m_Allocator->GetPhysicalDevice(), &MemoryProperties);

            for (std::uint32_t Iterator = 0u; Iterator < g_MaxFramesInFlight; ++Iterator)
            {
                CreateBuffer(MemoryProperties, BufferSize, UsageFlags, MemoryPropertyFlags, Object.UniformBuffers[Iterator].Buffer, Object.UniformBuffers[Iterator].Allocation);
                vmaMapMemory(m_Allocator, Object.UniformBuffers[Iterator].Allocation, &Object.UniformBuffers[Iterator].Data);
            }
        }
    }

    void UpdateUniformBuffers()
    {
        const std::uint8_t Frame = VulkanRenderSubsystem::Get()->GetFrameIndex();
        const VkExtent2D &SwapChainExtent = VulkanRenderSubsystem::Get()->GetDeviceProperties().Extent;

        if (Frame >= g_MaxFramesInFlight)
        {
            throw std::runtime_error("Vulkan image index is invalid.");
        }

        static auto StartTime = std::chrono::high_resolution_clock::now();

        const auto CurrentTime = std::chrono::high_resolution_clock::now();
        const float Time = std::chrono::duration<float, std::chrono::seconds::period>(CurrentTime - StartTime).count();

        for (auto &[ID, Object] : m_Objects)
        {
            if (Object.UniformBuffers.empty())
            {
                continue;
            }

            UniformBufferObject BufferObject{};
            BufferObject.Model = glm::rotate(glm::mat4(1.f), Time * glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f));
            BufferObject.View = glm::lookAt(glm::vec3(2.f, 2.f, 2.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 1.f));
            BufferObject.Projection = glm::perspective(glm::radians(45.0f), SwapChainExtent.width / (float)SwapChainExtent.height, 0.1f, 10.f);
            BufferObject.Projection[1][1] *= -1;

            std::memcpy(Object.UniformBuffers[Frame].Data, &BufferObject, sizeof(BufferObject));
        }
    }

    void CreateDepthResources()
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan depth resources";

        const VulkanDeviceProperties &Properties = VulkanRenderSubsystem::Get()->GetDeviceProperties();

        const VkQueue &GraphicsQueue = VulkanRenderSubsystem::Get()->GetQueueFromType(VulkanQueueType::Graphics);
        const std::uint8_t GraphicsQueueFamilyIndex = VulkanRenderSubsystem::Get()->GetQueueFamilyIndexFromType(VulkanQueueType::Graphics);

        CreateImage(Properties.DepthFormat, Properties.Extent, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage.Image, m_DepthImage.Allocation);
        CreateImageView(m_DepthImage.Image, Properties.DepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, m_DepthImage.View);
        MoveImageLayout(m_DepthImage.Image, Properties.DepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, GraphicsQueue, GraphicsQueueFamilyIndex);
    }

    std::uint64_t LoadObject(const std::string_view Path)
    {
        Assimp::Importer Importer;
        const aiScene *const Scene = Importer.ReadFile(Path.data(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

        if (!Scene || Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !Scene->mRootNode)
        {
            throw std::runtime_error("Assimp error: " + std::string(Importer.GetErrorString()));
        }
        
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loaded model from path: '" << Path << "'";

        static std::atomic<std::uint64_t> ObjectID = 0u;
        const std::uint64_t NewID = ObjectID.fetch_add(1u);
        m_Objects.emplace(NewID, VulkanObjectAllocation());
        VulkanObjectAllocation &NewObject = m_Objects.at(NewID);

        const std::vector<aiMesh*> Meshes(Scene->mMeshes, Scene->mMeshes + Scene->mNumMeshes);
        for (const aiMesh *MeshIter : Meshes)
        {
            for (std::uint32_t Iterator = 0u; Iterator < MeshIter->mNumVertices; ++Iterator)
            {
                const aiVector3D &Position = MeshIter->mVertices[Iterator];
                const aiVector3D &Normal = MeshIter->mNormals[Iterator];
                const aiVector3D &TextureCoord = MeshIter->mTextureCoords[0][Iterator];

                NewObject.Vertices.emplace_back(Vertex{
                    .Position = glm::vec3(Position.x, Position.y, Position.z),
                    .Normal = glm::vec3(Normal.x, Normal.y, Normal.z),
                    .Color = glm::vec3(1.f, 1.f, 1.f),
                    .TextureCoordinate = glm::vec2(TextureCoord.x, TextureCoord.y)});
            }

            for (std::uint32_t Iterator = 0u; Iterator < MeshIter->mNumFaces; ++Iterator)
            {
                const aiFace &Face = MeshIter->mFaces[Iterator];
                for (std::uint32_t FaceIterator = 0u; FaceIterator < Face.mNumIndices; ++FaceIterator)
                {
                    NewObject.Indices.emplace_back(Face.mIndices[FaceIterator]);
                }
            }
        }

        return NewID;
    }

    bool GetObjectTexture(const std::uint64_t ObjectID, std::vector<VulkanTextureData> &TextureData) const
    {
        if (!m_Objects.contains(ObjectID))
        {
            return false;
        }

        const VulkanObjectAllocation &Object = m_Objects.at(ObjectID);
        if (Object.TextureImages.empty())
        {
            return false;
        }

        TextureData.reserve(Object.TextureImages.size());
        for (const VulkanImageAllocation &ImageIter : Object.TextureImages)
        {
            TextureData.emplace_back(VulkanTextureData{
                .ImageView = ImageIter.View,
                .Sampler = ImageIter.Sampler});
        }

        return true;
    }

    void LoadTexture(const std::string_view Path, const std::uint64_t ObjectID)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan texture image";
        
        if (m_Allocator == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan memory allocator is invalid.");
        }

        std::int32_t Width;
        std::int32_t Height;
        std::int32_t Channels;

        stbi_uc *const ImagePixels = stbi_load(Path.data(), &Width, &Height, &Channels, STBI_rgb_alpha);
        const VkDeviceSize AllocationSize = Width * Height * 4u;

        if (!ImagePixels)
        {
            throw std::runtime_error("STB Image is invalid. Path: " + std::string(Path));
        }

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loaded image from path: '" << Path << "'";

        VkPhysicalDeviceMemoryProperties MemoryProperties;
        vkGetPhysicalDeviceMemoryProperties(m_Allocator->GetPhysicalDevice(), &MemoryProperties);

        VkBuffer StagingBuffer;
        VmaAllocation StagingBufferMemory;

        CreateBuffer(MemoryProperties, AllocationSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer, StagingBufferMemory);

        void *Data;
        vmaMapMemory(m_Allocator, StagingBufferMemory, &Data);
        std::memcpy(Data, ImagePixels, static_cast<std::size_t>(AllocationSize));
        vmaUnmapMemory(m_Allocator, StagingBufferMemory);

        stbi_image_free(ImagePixels);

        constexpr VkFormat DepthImageFormat = VK_FORMAT_R8G8B8A8_SRGB;
        constexpr VkImageLayout InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        constexpr VkImageLayout MiddleLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        constexpr VkImageLayout DestinationLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        const VkExtent2D Extent{
            .width = static_cast<std::uint32_t>(Width),
            .height = static_cast<std::uint32_t>(Height)};

        const VkQueue &GraphicsQueue = VulkanRenderSubsystem::Get()->GetQueueFromType(VulkanQueueType::Graphics);
        const std::uint8_t GraphicsQueueFamilyIndex = VulkanRenderSubsystem::Get()->GetQueueFamilyIndexFromType(VulkanQueueType::Graphics);

        constexpr VkImageTiling Tiling = VK_IMAGE_TILING_OPTIMAL;
        constexpr VkImageUsageFlags Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        constexpr VkMemoryPropertyFlags MemoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        VulkanImageAllocation NewAllocation;
        CreateImage(DepthImageFormat, Extent, Tiling, Usage, MemoryPropertyFlags, NewAllocation.Image, NewAllocation.Allocation);
        MoveImageLayout(NewAllocation.Image, DepthImageFormat, InitialLayout, MiddleLayout, GraphicsQueue, GraphicsQueueFamilyIndex);
        CopyBufferToImage(StagingBuffer, NewAllocation.Image, Extent, GraphicsQueue, GraphicsQueueFamilyIndex);
        MoveImageLayout(NewAllocation.Image, DepthImageFormat, MiddleLayout, DestinationLayout, GraphicsQueue, GraphicsQueueFamilyIndex);

        CreateTextureImageView(NewAllocation);
        CreateTextureSampler(NewAllocation);        
        vmaDestroyBuffer(m_Allocator, StagingBuffer, StagingBufferMemory);
        
        VulkanObjectAllocation &Object = m_Objects[ObjectID];
        Object.TextureImages.push_back(NewAllocation);
    }

    void Shutdown()
    {
        if (!IsInitialized())
        {
            return;
        }

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down Vulkan buffer manager";

        DestroyResources(true);

        vmaDestroyAllocator(m_Allocator);
        m_Allocator = VK_NULL_HANDLE;
    }

    void DestroyResources(const bool bClearScene)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying Vulkan buffer manager resources";

        const VkDevice &VulkanLogicalDevice = VulkanRenderSubsystem::Get()->GetDevice();

        if (m_SwapChain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(VulkanLogicalDevice, m_SwapChain, nullptr);
            m_SwapChain = VK_NULL_HANDLE;
        }

        for (VulkanImageAllocation &ImageViewIter : m_SwapChainImages)
        {
            ImageViewIter.DestroyResources(m_Allocator);
        }
        m_SwapChainImages.clear();

        for (VkFramebuffer &FrameBufferIter : m_FrameBuffers)
        {
            if (FrameBufferIter != VK_NULL_HANDLE)
            {
                vkDestroyFramebuffer(VulkanLogicalDevice, FrameBufferIter, nullptr);
                FrameBufferIter = VK_NULL_HANDLE;
            }
        }
        m_FrameBuffers.clear();
        
        m_DepthImage.DestroyResources(m_Allocator);

        for (auto &[ObjectID, ObjectIter] : m_Objects)
        {
            ObjectIter.DestroyResources(bClearScene, m_Allocator);
        }

        if (bClearScene)
        {
            m_Objects.clear();
        }
    }

    bool IsInitialized() const
    {
        return m_Allocator != VK_NULL_HANDLE;
    }

    const VkSwapchainKHR &GetSwapChain() const
    {
        return m_SwapChain;
    }

    const std::vector<VkImage> GetSwapChainImages() const
    {
        std::vector<VkImage> SwapChainImages;
        SwapChainImages.reserve(m_SwapChainImages.size());

        for (const VulkanImageAllocation &ImageIter : m_SwapChainImages)
        {
            SwapChainImages.emplace_back(ImageIter.Image);
        }

        return SwapChainImages;
    }

    const std::vector<VkFramebuffer> &GetFrameBuffers() const
    {
        return m_FrameBuffers;
    }

    const std::vector<VkBuffer> GetVertexBuffers(const std::uint64_t ObjectID) const
    {
        std::vector<VkBuffer> VertexBuffers;
        VertexBuffers.reserve(m_Objects.at(ObjectID).VertexBuffers.size());

        for (const VulkanBufferAllocation &BufferIter : m_Objects.at(ObjectID).VertexBuffers)
        {
            VertexBuffers.emplace_back(BufferIter.Buffer);
        }

        return VertexBuffers;
    }

    const std::vector<VkBuffer> GetIndexBuffers(const std::uint64_t ObjectID) const
    {
        std::vector<VkBuffer> IndexBuffers;
        IndexBuffers.reserve(m_Objects.at(ObjectID).IndexBuffers.size());

        for (const VulkanBufferAllocation &BufferIter : m_Objects.at(ObjectID).IndexBuffers)
        {
            IndexBuffers.emplace_back(BufferIter.Buffer);
        }

        return IndexBuffers;
    }

    const std::vector<VkBuffer> GetUniformBuffers(const std::uint64_t ObjectID) const
    {
        std::vector<VkBuffer> UniformBuffers;
        UniformBuffers.reserve(m_Objects.at(ObjectID).UniformBuffers.size());

        for (const VulkanBufferAllocation &BufferIter : m_Objects.at(ObjectID).UniformBuffers)
        {
            UniformBuffers.emplace_back(BufferIter.Buffer);
        }

        return UniformBuffers;
    }

    const std::vector<Vertex> &GetVertices(const std::uint64_t ObjectID) const
    {
        return m_Objects.at(ObjectID).Vertices;
    }

    const std::vector<std::uint32_t> &GetIndices(const std::uint64_t ObjectID) const
    {
        return m_Objects.at(ObjectID).Indices;
    }

    std::uint32_t GetIndicesCount(const std::uint64_t ObjectID) const
    {
        return static_cast<std::uint32_t>(m_Objects.at(ObjectID).Indices.size());
    }

private:    
    std::uint32_t FindMemoryType(const VkPhysicalDeviceMemoryProperties &Properties, const std::uint32_t &TypeFilter, const VkMemoryPropertyFlags &Flags) const
    {
        for (std::uint32_t Iterator = 0u; Iterator < Properties.memoryTypeCount; ++Iterator)
        {
            if ((TypeFilter & (1 << Iterator)) && (Properties.memoryTypes[Iterator].propertyFlags & Flags) == Flags)
            {
                return Iterator;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type.");
    }

    void CreateBuffer(const VkPhysicalDeviceMemoryProperties &Properties, const VkDeviceSize &Size, const VkBufferUsageFlags Usage, const VkMemoryPropertyFlags Flags, VkBuffer &Buffer, VmaAllocation &Allocation) const
    {
        if (m_Allocator == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan memory allocator is invalid.");
        }

        const VkBufferCreateInfo BufferCreateInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = Size,
            .usage = Usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        const VmaAllocationCreateInfo AllocationCreateInfo{
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
            .requiredFlags = Flags | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        };

        VmaAllocationInfo MemoryAllocationInfo;
        RENDERCORE_CHECK_VULKAN_RESULT(vmaCreateBuffer(m_Allocator, &BufferCreateInfo, &AllocationCreateInfo, &Buffer, &Allocation, &MemoryAllocationInfo));
    }

    void CopyBuffer(const VkBuffer &Source, const VkBuffer &Destination, const VkDeviceSize &Size, const VkQueue &Queue, const std::uint8_t QueueFamilyIndex) const
    {
        VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
        VkCommandPool CommandPool = VK_NULL_HANDLE;
        InitializeSingleCommandQueue(CommandPool, CommandBuffer, QueueFamilyIndex);
        {
            const VkBufferCopy BufferCopy{
                .size = Size,
            };

            vkCmdCopyBuffer(CommandBuffer, Source, Destination, 1u, &BufferCopy);    
        }
        FinishSingleCommandQueue(Queue, CommandPool, CommandBuffer);
    }

    void CreateImage(const VkFormat &ImageFormat, const VkExtent2D& Extent, const VkImageTiling& Tiling, const VkImageUsageFlags Usage, const VkMemoryPropertyFlags Flags, VkImage& Image, VmaAllocation& Allocation)
    {
        if (m_Allocator == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan memory allocator is invalid.");
        }

        const VkImageCreateInfo ImageViewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = ImageFormat,
            .extent = {.width = Extent.width, .height = Extent.height, .depth = 1u},
            .mipLevels = 1u,
            .arrayLayers = 1u,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = Tiling,
            .usage = Usage,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};

        const VmaAllocationCreateInfo ImageAllocationCreateInfo{
            .usage = VMA_MEMORY_USAGE_AUTO,
            .requiredFlags = Flags};

        VmaAllocationInfo AllocationInfo;
        RENDERCORE_CHECK_VULKAN_RESULT(vmaCreateImage(m_Allocator, &ImageViewCreateInfo, &ImageAllocationCreateInfo, &Image, &Allocation, &AllocationInfo));
    }

    void CreateImageView(const VkImage &Image, const VkFormat &Format, const VkImageAspectFlags &AspectFlags, VkImageView &ImageView) const
    {
        const VkImageViewCreateInfo ImageViewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = Image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = Format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY},
            .subresourceRange = {
                .aspectMask = AspectFlags,
                .baseMipLevel = 0u,
                .levelCount = 1u,
                .baseArrayLayer = 0u,
                .layerCount = 1u}};

        RENDERCORE_CHECK_VULKAN_RESULT(vkCreateImageView(VulkanRenderSubsystem::Get()->GetDevice(), &ImageViewCreateInfo, nullptr, &ImageView));
    }

    void CreateTextureSampler(VulkanImageAllocation& Allocation)
    {
        if (m_Allocator == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan memory allocator is invalid.");
        }

        VkPhysicalDeviceProperties DeviceProperties;
        vkGetPhysicalDeviceProperties(m_Allocator->GetPhysicalDevice(), &DeviceProperties);

        const VkSamplerCreateInfo SamplerCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias = 0.f,
            .anisotropyEnable = VK_TRUE,
            .maxAnisotropy = DeviceProperties.limits.maxSamplerAnisotropy,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .minLod = 0.f,
            .maxLod = 0.f,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE};

        RENDERCORE_CHECK_VULKAN_RESULT(vkCreateSampler(VulkanRenderSubsystem::Get()->GetDevice(), &SamplerCreateInfo, nullptr, &Allocation.Sampler));
    }

    void CopyBufferToImage(const VkBuffer &Source, const VkImage &Destination, const VkExtent2D &Extent, const VkQueue &Queue, const std::uint32_t QueueFamilyIndex) const
    {
        VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
        VkCommandPool CommandPool = VK_NULL_HANDLE;
        InitializeSingleCommandQueue(CommandPool, CommandBuffer, QueueFamilyIndex);
        {
            const VkBufferImageCopy BufferImageCopy{
                .bufferOffset = 0u,
                .bufferRowLength = 0u,
                .bufferImageHeight = 0u,
                .imageSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0u,
                    .baseArrayLayer = 0u,
                    .layerCount = 1u},
                .imageOffset = {.x = 0u, .y = 0u, .z = 0u},
                .imageExtent = {.width = Extent.width, .height = Extent.height, .depth = 1u}};

            vkCmdCopyBufferToImage(CommandBuffer, Source, Destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &BufferImageCopy);
        }
        FinishSingleCommandQueue(Queue, CommandPool, CommandBuffer);
    }

    void MoveImageLayout(const VkImage &Image, const VkFormat &Format, const VkImageLayout &OldLayout, const VkImageLayout &NewLayout, const VkQueue &Queue, const std::uint8_t QueueFamilyIndex)
    {
        VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
        VkCommandPool CommandPool = VK_NULL_HANDLE;
        InitializeSingleCommandQueue(CommandPool, CommandBuffer, QueueFamilyIndex);
        {
            VkPipelineStageFlags SourceStage;
            VkPipelineStageFlags DestinationStage;

            VkImageMemoryBarrier Barrier = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .oldLayout = OldLayout,
                .newLayout = NewLayout,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = Image,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0u,
                    .levelCount = 1u,
                    .baseArrayLayer = 0u,
                    .layerCount = 1u}};

            if (NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
            {
                Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

                if (Format == VK_FORMAT_D32_SFLOAT_S8_UINT || Format == VK_FORMAT_D24_UNORM_S8_UINT)
                {
                    Barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                }
            }

            if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                Barrier.srcAccessMask = 0;
                Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                SourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                DestinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                SourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                DestinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
            {
                Barrier.srcAccessMask = 0;
                Barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                SourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                DestinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            }
            else
            {
                throw std::invalid_argument("Vulkan image layout transition is invalid");
            }

            vkCmdPipelineBarrier(CommandBuffer, SourceStage, DestinationStage, 0u, 0u, nullptr, 0u, nullptr, 1u, &Barrier);
        }
        FinishSingleCommandQueue(Queue, CommandPool, CommandBuffer);
    }

    void CreateTextureImageView(VulkanImageAllocation& Allocation)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan texture image views";
        CreateImageView(Allocation.Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, Allocation.View);
    }

    void CreateSwapChainImageViews(const VkFormat &ImageFormat)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan swap chain image views";
        for (VulkanImageAllocation& ImageIter : m_SwapChainImages)
        {
            CreateImageView(ImageIter.Image, ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, ImageIter.View);
        }
    }

    VmaAllocator m_Allocator;
    VkSwapchainKHR m_SwapChain;
    std::vector<VulkanImageAllocation> m_SwapChainImages;
    VulkanImageAllocation m_DepthImage;
    std::vector<VkFramebuffer> m_FrameBuffers;
    std::unordered_map<std::uint64_t, VulkanObjectAllocation> m_Objects;
};

VulkanBufferManager::VulkanBufferManager()
    : m_Impl(std::make_unique<VulkanBufferManager::Impl>())
{
}

VulkanBufferManager::~VulkanBufferManager()
{
    Shutdown();
}

void VulkanBufferManager::CreateMemoryAllocator()
{
    m_Impl->CreateMemoryAllocator();
}

void VulkanBufferManager::CreateSwapChain(const bool bRecreate)
{
    m_Impl->CreateSwapChain(bRecreate);
}

void VulkanBufferManager::CreateDepthResources()
{
    m_Impl->CreateDepthResources();
}

std::uint64_t VulkanBufferManager::LoadObject(const std::string_view Path)
{
    return m_Impl->LoadObject(Path);
}

void VulkanBufferManager::LoadTexture(const std::string_view Path, const std::uint64_t ObjectID)
{
    return m_Impl->LoadTexture(Path, ObjectID);
}

void VulkanBufferManager::UpdateUniformBuffers()
{
    m_Impl->UpdateUniformBuffers();
}

void VulkanBufferManager::CreateFrameBuffers(const VkRenderPass &RenderPass)
{
    m_Impl->CreateFrameBuffers(RenderPass);
}

void VulkanBufferManager::DestroyResources(const bool bClearScene)
{
    m_Impl->DestroyResources(bClearScene);
}

void VulkanBufferManager::Shutdown()
{
    m_Impl->Shutdown();
}

bool VulkanBufferManager::IsInitialized() const
{
    return m_Impl->IsInitialized();
}

const VkSwapchainKHR &VulkanBufferManager::GetSwapChain() const
{
    return m_Impl->GetSwapChain();
}

const std::vector<VkImage> VulkanBufferManager::GetSwapChainImages() const
{
    return m_Impl->GetSwapChainImages();
}

const std::vector<VkFramebuffer> &VulkanBufferManager::GetFrameBuffers() const
{
    return m_Impl->GetFrameBuffers();
}

const std::vector<VkBuffer> VulkanBufferManager::GetVertexBuffers(const std::uint64_t ObjectID) const
{
    return m_Impl->GetVertexBuffers(ObjectID);
}

const std::vector<VkBuffer> VulkanBufferManager::GetIndexBuffers(const std::uint64_t ObjectID) const
{
    return m_Impl->GetIndexBuffers(ObjectID);
}

const std::vector<VkBuffer> VulkanBufferManager::GetUniformBuffers(const std::uint64_t ObjectID) const
{
    return m_Impl->GetUniformBuffers(ObjectID);
}

const std::vector<Vertex> VulkanBufferManager::GetVertices(const std::uint64_t ObjectID) const
{
    return m_Impl->GetVertices(ObjectID);
}

const std::vector<std::uint32_t> VulkanBufferManager::GetIndices(const std::uint64_t ObjectID) const
{
    return m_Impl->GetIndices(ObjectID);
}

const std::uint32_t VulkanBufferManager::GetIndicesCount(const std::uint64_t ObjectID) const
{
    return m_Impl->GetIndicesCount(ObjectID);
}

const bool VulkanBufferManager::GetObjectTexture(const std::uint64_t ObjectID, std::vector<VulkanTextureData> &TextureData) const
{
    return m_Impl->GetObjectTexture(ObjectID, TextureData);
}
