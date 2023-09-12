// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "Managers/VulkanBufferManager.h"
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

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

using namespace RenderCore;

class VulkanBufferManager::Impl
{
public:
    Impl() = delete;
    Impl(const VulkanBufferManager &) = delete;
    Impl &operator=(const VulkanBufferManager &) = delete;

    Impl(const VkDevice &Device, const VkSurfaceKHR &Surface, const std::vector<std::uint32_t> &QueueFamilyIndices)
        : m_Allocator(VK_NULL_HANDLE)
        , m_Device(Device)
        , m_Surface(Surface)
        , m_QueueFamilyIndices(QueueFamilyIndices)
        , m_SwapChain(VK_NULL_HANDLE)
        , m_SwapChainImages({})
        , m_SwapChainImageViews({})
        , m_TextureImages({})
        , m_TextureImagesMemory({})
        , m_TextureImageViews({})
        , m_TextureSamplers({})
        , m_DepthImage(VK_NULL_HANDLE)
        , m_DepthImageMemory(VK_NULL_HANDLE)
        , m_DepthImageView(VK_NULL_HANDLE)
        , m_FrameBuffers({})
        , m_VertexBuffers({})
        , m_VertexBuffersMemory({})
        , m_IndexBuffers({})
        , m_IndexBuffersMemory({})
        , m_UniformBuffers({})
        , m_UniformBuffersMemory({})
        , m_UniformBuffersData({})
        , m_Vertices({})
        , m_Indices({})
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
    
    void CreateMemoryAllocator(const VkInstance &Instance, const VkDevice& LogicalDevice, const VkPhysicalDevice& PhysicalDevice)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan memory allocator";

        const VmaAllocatorCreateInfo AllocatorInfo{
            .flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT,
            .physicalDevice = PhysicalDevice,
            .device = LogicalDevice,
            .preferredLargeHeapBlockSize = 0u /*Default: 256 MiB*/,
            .pAllocationCallbacks = nullptr,
            .pDeviceMemoryCallbacks = nullptr,
            .pHeapSizeLimit = nullptr,
            .pVulkanFunctions = nullptr,
            .instance = Instance,
            .vulkanApiVersion = VK_API_VERSION_1_0,
            .pTypeExternalMemoryHandleTypes = nullptr
        };

        RENDERCORE_CHECK_VULKAN_RESULT(vmaCreateAllocator(&AllocatorInfo, &m_Allocator));
    }

    void CreateSwapChain(const VkSurfaceFormatKHR &PreferredFormat, const VkPresentModeKHR &PreferredMode, const VkExtent2D &PreferredExtent, const VkSurfaceCapabilitiesKHR &Capabilities)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan swap chain";

        if (IsInitialized())
        {
            DestroyResources();
        }

        if (m_Device == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan logical device is invalid.");
        }

        if (m_Surface == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan surface is invalid.");
        }

        const std::uint32_t QueueFamilyIndicesCount = static_cast<std::uint32_t>(m_QueueFamilyIndices.size());
        const std::uint32_t MinImageCount = Capabilities.minImageCount < 3u && Capabilities.maxImageCount >= 3u ? 3u : Capabilities.minImageCount;
        
        const VkSwapchainCreateInfoKHR SwapChainCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = m_Surface,
            .minImageCount = MinImageCount,
            .imageFormat = PreferredFormat.format,
            .imageColorSpace = PreferredFormat.colorSpace,
            .imageExtent = PreferredExtent,
            .imageArrayLayers = 1u,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = QueueFamilyIndicesCount > 1u ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = QueueFamilyIndicesCount,
            .pQueueFamilyIndices = m_QueueFamilyIndices.data(),
            .preTransform = Capabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = PreferredMode,
            .clipped = VK_TRUE,
        };

        RENDERCORE_CHECK_VULKAN_RESULT(vkCreateSwapchainKHR(m_Device, &SwapChainCreateInfo, nullptr, &m_SwapChain));

        std::uint32_t Count = 0u;
        RENDERCORE_CHECK_VULKAN_RESULT(vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &Count, nullptr));

        m_SwapChainImages.resize(Count, VK_NULL_HANDLE);
        RENDERCORE_CHECK_VULKAN_RESULT(vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &Count, m_SwapChainImages.data()));

        CreateSwapChainImageViews(PreferredFormat.format);
    }

    void CreateFrameBuffers(const VkRenderPass &RenderPass, const VkExtent2D &Extent)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan frame buffers";

        if (m_Device == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan logical device is invalid.");
        }

        if (RenderPass == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan render pass is invalid.");
        }

        m_FrameBuffers.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);

        for (std::uint32_t Iterator = 0u; Iterator < static_cast<std::uint32_t>(m_FrameBuffers.size()); ++Iterator)
        {
            const std::array<VkImageView, 2u> Attachments{
                m_SwapChainImageViews[Iterator],
                m_DepthImageView};

            const VkFramebufferCreateInfo FrameBufferCreateInfo{
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = RenderPass,
                .attachmentCount = static_cast<std::uint32_t>(Attachments.size()),
                .pAttachments = Attachments.data(),
                .width = Extent.width,
                .height = Extent.height,
                .layers = 1u};

            RENDERCORE_CHECK_VULKAN_RESULT(vkCreateFramebuffer(m_Device, &FrameBufferCreateInfo, nullptr, &m_FrameBuffers[Iterator]));
        }
    }

    void CreateVertexBuffers(const VkQueue &Queue, const std::uint32_t QueueFamilyIndex)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan vertex buffers";

        if (m_Allocator == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan memory allocator is invalid.");
        }

        m_VertexBuffers.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);
        m_VertexBuffersMemory.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);

        const VkDeviceSize BufferSize = m_Vertices.size() * sizeof(Vertex);

        VkPhysicalDeviceMemoryProperties MemoryProperties;
        vkGetPhysicalDeviceMemoryProperties(m_Allocator->GetPhysicalDevice(), &MemoryProperties);

        for (std::uint32_t Iterator = 0u; Iterator < static_cast<std::uint32_t>(m_VertexBuffers.size()); ++Iterator)
        {
            VkBuffer StagingBuffer;
            VmaAllocation StagingBufferMemory;
            CreateBuffer(MemoryProperties, BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer, StagingBufferMemory);

            void *Data;
            vmaMapMemory(m_Allocator, StagingBufferMemory, &Data);
            std::memcpy(Data, m_Vertices.data(), static_cast<std::size_t>(BufferSize));
            vmaUnmapMemory(m_Allocator, StagingBufferMemory);

            CreateBuffer(MemoryProperties, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffers[Iterator], m_VertexBuffersMemory[Iterator]);
            CopyBuffer(StagingBuffer, m_VertexBuffers[Iterator], BufferSize, Queue, QueueFamilyIndex);

            vmaDestroyBuffer(m_Allocator, StagingBuffer, StagingBufferMemory);
        }
    }

    void CreateIndexBuffers(const VkQueue &Queue, const std::uint32_t QueueFamilyIndex)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan index buffers";

        if (m_Allocator == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan memory allocator is invalid.");
        }

        m_IndexBuffers.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);
        m_IndexBuffersMemory.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);

        const VkDeviceSize BufferSize = m_Indices.size() * sizeof(std::uint32_t);

        VkPhysicalDeviceMemoryProperties MemoryProperties;
        vkGetPhysicalDeviceMemoryProperties(m_Allocator->GetPhysicalDevice(), &MemoryProperties);

        for (std::uint32_t Iterator = 0u; Iterator < static_cast<std::uint32_t>(m_IndexBuffers.size()); ++Iterator)
        {
            VkBuffer StagingBuffer;
            VmaAllocation StagingBufferMemory;
            CreateBuffer(MemoryProperties, BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer, StagingBufferMemory);

            void *Data;
            vmaMapMemory(m_Allocator, StagingBufferMemory, &Data);
            std::memcpy(Data, m_Indices.data(), static_cast<std::size_t>(BufferSize));
            vmaUnmapMemory(m_Allocator, StagingBufferMemory);

            CreateBuffer(MemoryProperties, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBuffers[Iterator], m_IndexBuffersMemory[Iterator]);
            CopyBuffer(StagingBuffer, m_IndexBuffers[Iterator], BufferSize, Queue, QueueFamilyIndex);

            vmaDestroyBuffer(m_Allocator, StagingBuffer, StagingBufferMemory);
        }
    }

    void CreateUniformBuffers()
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan index buffers";

        if (m_Allocator == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan memory allocator is invalid.");
        }

        m_UniformBuffers.resize(g_MaxFramesInFlight, VK_NULL_HANDLE);
        m_UniformBuffersMemory.resize(g_MaxFramesInFlight, VK_NULL_HANDLE);
        m_UniformBuffersData.resize(g_MaxFramesInFlight, nullptr);

        const VkDeviceSize BufferSize = sizeof(UniformBufferObject);

        VkPhysicalDeviceMemoryProperties MemoryProperties;
        vkGetPhysicalDeviceMemoryProperties(m_Allocator->GetPhysicalDevice(), &MemoryProperties);

        for (std::uint32_t Iterator = 0u; Iterator < g_MaxFramesInFlight; ++Iterator)
        {
            CreateBuffer(MemoryProperties, BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_UniformBuffers[Iterator], m_UniformBuffersMemory[Iterator]);
            vmaMapMemory(m_Allocator, m_UniformBuffersMemory[Iterator], &m_UniformBuffersData[Iterator]);
        }
    }

    void UpdateUniformBuffers(const std::uint32_t Frame, const VkExtent2D &SwapChainExtent)
    {
        if (Frame >= g_MaxFramesInFlight)
        {
            throw std::runtime_error("Vulkan image index is invalid.");
        }

        static auto StartTime = std::chrono::high_resolution_clock::now();

        const auto CurrentTime = std::chrono::high_resolution_clock::now();
        const float Time = std::chrono::duration<float, std::chrono::seconds::period>(CurrentTime - StartTime).count();

        UniformBufferObject BufferObject{};
        BufferObject.Model = glm::rotate(glm::mat4(1.f), Time * glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f));
        BufferObject.View = glm::lookAt(glm::vec3(2.f, 2.f, 2.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 1.f));
        BufferObject.Projection = glm::perspective(glm::radians(45.0f), SwapChainExtent.width / (float)SwapChainExtent.height, 0.1f, 10.f);
        BufferObject.Projection[1][1] *= -1;

        std::memcpy(m_UniformBuffersData[Frame], &BufferObject, sizeof(BufferObject));
    }

    void CreateTextureImage(const std::string_view Path, const VkQueue &Queue, const std::uint32_t QueueFamilyIndex, VkImageView& ImageView, VkSampler& Sampler)
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

        m_TextureImages.emplace_back(VK_NULL_HANDLE);
        m_TextureImagesMemory.emplace_back(VK_NULL_HANDLE);
        CreateImage(VK_FORMAT_R8G8B8A8_SRGB, {.width = static_cast<std::uint32_t>(Width), .height = static_cast<std::uint32_t>(Height)}, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImages.back(), m_TextureImagesMemory.back());
        MoveImageLayout(m_TextureImages.back(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Queue, QueueFamilyIndex);
        CopyBufferToImage(StagingBuffer, m_TextureImages.back(), {.width = static_cast<std::uint32_t>(Width), .height = static_cast<std::uint32_t>(Height)}, Queue, QueueFamilyIndex);
        MoveImageLayout(m_TextureImages.back(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Queue, QueueFamilyIndex);

        CreateTextureImageView();
        CreateTextureSampler();

        ImageView = m_TextureImageViews.back();
        Sampler = m_TextureSamplers.back();
        
        vmaDestroyBuffer(m_Allocator, StagingBuffer, StagingBufferMemory);
    }

    void CreateDepthResources(const VkFormat &Format, const VkExtent2D &Extent, const VkQueue &Queue, const std::uint32_t QueueFamilyIndex)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan depth resources";

        CreateImage(Format, Extent, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage, m_DepthImageMemory);
        CreateImageView(m_DepthImage, Format, VK_IMAGE_ASPECT_DEPTH_BIT, m_DepthImageView);
        MoveImageLayout(m_DepthImage, Format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, Queue, QueueFamilyIndex);
    }

    void LoadScene(const std::string_view Path)
    {
        Assimp::Importer Importer;
        const aiScene *const Scene = Importer.ReadFile(Path.data(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

        if (!Scene || Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !Scene->mRootNode)
        {
            throw std::runtime_error("Assimp error: " + std::string(Importer.GetErrorString()));
        }

        const std::vector<aiMesh*> Meshes(Scene->mMeshes, Scene->mMeshes + Scene->mNumMeshes);
        for (const aiMesh *MeshIter : Meshes)
        {
            for (std::uint32_t Iterator = 0u; Iterator < MeshIter->mNumVertices; ++Iterator)
            {
                const aiVector3D &Position = MeshIter->mVertices[Iterator];
                const aiVector3D &Normal = MeshIter->mNormals[Iterator];
                const aiVector3D &TextureCoord = MeshIter->mTextureCoords[0][Iterator];

                m_Vertices.emplace_back(Vertex{
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
                    m_Indices.emplace_back(Face.mIndices[FaceIterator]);
                }
            }
        }
    }

    void Shutdown()
    {
        if (!IsInitialized())
        {
            return;
        }

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down Vulkan buffer manager";
        DestroyResources();        

        for (std::uint32_t Iterator = 0u; Iterator < static_cast<std::uint32_t>(m_TextureImages.size()); ++Iterator)
        {
            vmaDestroyImage(m_Allocator, m_TextureImages[Iterator], m_TextureImagesMemory[Iterator]);
        }
        m_TextureImages.clear();
        m_TextureImagesMemory.clear();

        for (const VkImageView &ImageViewIter : m_TextureImageViews)
        {
            if (ImageViewIter != VK_NULL_HANDLE)
            {
                vkDestroyImageView(m_Device, ImageViewIter, nullptr);
            }
        }
        m_TextureImageViews.clear();

        for (const VkSampler &SamplerIter : m_TextureSamplers)
        {
            if (SamplerIter != VK_NULL_HANDLE)
            {
                vkDestroySampler(m_Device, SamplerIter, nullptr);
            }
        }
        m_TextureSamplers.clear();

        for (std::uint32_t Iterator = 0u; Iterator < static_cast<std::uint32_t>(m_VertexBuffers.size()); ++Iterator)
        {
            vmaDestroyBuffer(m_Allocator, m_VertexBuffers[Iterator], m_VertexBuffersMemory[Iterator]);
        }
        m_VertexBuffers.clear();
        m_VertexBuffersMemory.clear();

        for (std::uint32_t Iterator = 0u; Iterator < static_cast<std::uint32_t>(m_IndexBuffers.size()); ++Iterator)
        {
            vmaDestroyBuffer(m_Allocator, m_IndexBuffers[Iterator], m_IndexBuffersMemory[Iterator]);
        }
        m_IndexBuffers.clear();
        m_IndexBuffersMemory.clear();

        for (std::uint32_t Iterator = 0u; Iterator < static_cast<std::uint32_t>(m_UniformBuffers.size()); ++Iterator)
        {
            vmaUnmapMemory(m_Allocator, m_UniformBuffersMemory[Iterator]);
            vmaDestroyBuffer(m_Allocator, m_UniformBuffers[Iterator], m_UniformBuffersMemory[Iterator]);
        }
        m_UniformBuffers.clear();
        m_UniformBuffersMemory.clear();
        m_UniformBuffersData.clear();

        vmaDestroyAllocator(m_Allocator);
        m_Allocator = VK_NULL_HANDLE;
    }

    void DestroyResources()
    {
        if (m_Device == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan logical device is invalid.");
        }

        if (m_SwapChain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
            m_SwapChain = VK_NULL_HANDLE;
        }

        if (m_DepthImage != VK_NULL_HANDLE)
        {
            vmaDestroyImage(m_Allocator, m_DepthImage, m_DepthImageMemory);
            m_DepthImage = VK_NULL_HANDLE;
            m_DepthImageMemory = VK_NULL_HANDLE;
        }

        if (m_DepthImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_Device, m_DepthImageView, nullptr);
            m_DepthImageView = VK_NULL_HANDLE;
        }

        for (const VkImageView &ImageViewIter : m_SwapChainImageViews)
        {
            if (ImageViewIter != VK_NULL_HANDLE)
            {
                vkDestroyImageView(m_Device, ImageViewIter, nullptr);
            }
        }
        m_SwapChainImages.clear();
        m_SwapChainImageViews.clear();

        for (const VkFramebuffer &FrameBufferIter : m_FrameBuffers)
        {
            if (FrameBufferIter != VK_NULL_HANDLE)
            {
                vkDestroyFramebuffer(m_Device, FrameBufferIter, nullptr);
            }
        }
        m_FrameBuffers.clear();
    }

    bool IsInitialized() const
    {
        return m_Device != VK_NULL_HANDLE && m_Surface != VK_NULL_HANDLE;
    }

    const VkSwapchainKHR &GetSwapChain() const
    {
        return m_SwapChain;
    }

    const std::vector<VkImage> &GetSwapChainImages() const
    {
        return m_SwapChainImages;
    }

    const std::vector<VkFramebuffer> &GetFrameBuffers() const
    {
        return m_FrameBuffers;
    }

    const std::vector<VkBuffer> &GetVertexBuffers() const
    {
        return m_VertexBuffers;
    }

    const std::vector<VmaAllocation> &GetVertexBuffersMemory() const
    {
        return m_VertexBuffersMemory;
    }

    const std::vector<VkBuffer> &GetIndexBuffers() const
    {
        return m_IndexBuffers;
    }

    const std::vector<VmaAllocation> &GetIndexBuffersMemory() const
    {
        return m_IndexBuffersMemory;
    }

    const std::vector<VkBuffer> &GetUniformBuffers() const
    {
        return m_UniformBuffers;
    }

    const std::vector<VmaAllocation> &GetUniformBuffersMemory() const
    {
        return m_UniformBuffersMemory;
    }

    const std::vector<void *> &GetUniformBuffersData() const
    {
        return m_UniformBuffersData;
    }

    const std::vector<Vertex> &GetVertices() const
    {
        return m_Vertices;
    }

    const std::vector<std::uint32_t> &GetIndices() const
    {
        return m_Indices;
    }

    std::uint32_t GetIndicesCount() const
    {
        return static_cast<std::uint32_t>(m_Indices.size());
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

        if (m_Device == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan logical device is invalid.");
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

    void CopyBuffer(const VkBuffer &Source, const VkBuffer &Destination, const VkDeviceSize &Size, const VkQueue &Queue, const std::uint32_t QueueFamilyIndex) const
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
        if (m_Device == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan logical device is invalid.");
        }

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

        RENDERCORE_CHECK_VULKAN_RESULT(vkCreateImageView(m_Device, &ImageViewCreateInfo, nullptr, &ImageView));
    }

    void CreateTextureSampler()
    {
        if (m_Allocator == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan memory allocator is invalid.");
        }

        if (m_Device == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan logical device is invalid.");
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

        m_TextureSamplers.emplace_back(VK_NULL_HANDLE);
        RENDERCORE_CHECK_VULKAN_RESULT(vkCreateSampler(m_Device, &SamplerCreateInfo, nullptr, &m_TextureSamplers.back()));
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

    void MoveImageLayout(const VkImage &Image, const VkFormat &Format, const VkImageLayout &OldLayout, const VkImageLayout &NewLayout, const VkQueue &Queue, const std::uint32_t QueueFamilyIndex)
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

    void InitializeSingleCommandQueue(VkCommandPool &CommandPool, VkCommandBuffer &CommandBuffer, const std::uint32_t QueueFamilyIndex) const
    {
        if (m_Device == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan logical device is invalid.");
        }

        const VkCommandPoolCreateInfo CommandPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = QueueFamilyIndex};

        RENDERCORE_CHECK_VULKAN_RESULT(vkCreateCommandPool(m_Device, &CommandPoolCreateInfo, nullptr, &CommandPool));

        const VkCommandBufferBeginInfo CommandBufferBeginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        const VkCommandBufferAllocateInfo CommandBufferAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = CommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1u,
        };

        RENDERCORE_CHECK_VULKAN_RESULT(vkAllocateCommandBuffers(m_Device, &CommandBufferAllocateInfo, &CommandBuffer));
        RENDERCORE_CHECK_VULKAN_RESULT(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));        
    }

    void FinishSingleCommandQueue(const VkQueue &Queue, const VkCommandPool &CommandPool, const VkCommandBuffer &CommandBuffer) const
    {
        if (m_Device == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan logical device is invalid.");
        }

        if (CommandPool == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan command pool is invalid.");
        }

        if (CommandBuffer == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan command buffer is invalid.");
        }

        RENDERCORE_CHECK_VULKAN_RESULT(vkEndCommandBuffer(CommandBuffer));

        const VkSubmitInfo SubmitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1u,
            .pCommandBuffers = &CommandBuffer,
        };

        RENDERCORE_CHECK_VULKAN_RESULT(vkQueueSubmit(Queue, 1u, &SubmitInfo, VK_NULL_HANDLE));
        RENDERCORE_CHECK_VULKAN_RESULT(vkQueueWaitIdle(Queue));

        vkFreeCommandBuffers(m_Device, CommandPool, 1u, &CommandBuffer);
        vkDestroyCommandPool(m_Device, CommandPool, nullptr);
    }

    void CreateTextureImageView()
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan texture image views";

        m_TextureImageViews.emplace_back(VK_NULL_HANDLE);
        CreateImageView(m_TextureImages.back(), VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, m_TextureImageViews.back());
    }

    void CreateSwapChainImageViews(const VkFormat &ImageFormat)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan swap chain image views";

        m_SwapChainImageViews.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);
        for (auto ImageIter = m_SwapChainImages.begin(); ImageIter != m_SwapChainImages.end(); ++ImageIter)
        {
            const std::int32_t Index = std::distance(m_SwapChainImages.begin(), ImageIter);
            CreateImageView(*ImageIter, ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, m_SwapChainImageViews[Index]);
        }
    }

    const VkDevice &m_Device;
    const VkSurfaceKHR &m_Surface;
    const std::vector<std::uint32_t> m_QueueFamilyIndices;

    VmaAllocator m_Allocator;
    VkSwapchainKHR m_SwapChain;
    std::vector<VkImage> m_SwapChainImages;
    std::vector<VkImageView> m_SwapChainImageViews;

    std::vector<VkImage> m_TextureImages;
    std::vector<VmaAllocation> m_TextureImagesMemory;
    std::vector<VkImageView> m_TextureImageViews;
    std::vector<VkSampler> m_TextureSamplers;

    VkImage m_DepthImage;
    VmaAllocation m_DepthImageMemory;
    VkImageView m_DepthImageView;

    std::vector<VkFramebuffer> m_FrameBuffers;

    std::vector<VkBuffer> m_VertexBuffers;
    std::vector<VmaAllocation> m_VertexBuffersMemory;

    std::vector<VkBuffer> m_IndexBuffers;
    std::vector<VmaAllocation> m_IndexBuffersMemory;

    std::vector<VkBuffer> m_UniformBuffers;
    std::vector<VmaAllocation> m_UniformBuffersMemory;
    std::vector<void *> m_UniformBuffersData;

    std::vector<Vertex> m_Vertices;
    std::vector<std::uint32_t> m_Indices;
};

VulkanBufferManager::VulkanBufferManager(const VkDevice &Device, const VkSurfaceKHR &Surface, const std::vector<std::uint32_t> &QueueFamilyIndices)
    : m_Impl(std::make_unique<VulkanBufferManager::Impl>(Device, Surface, QueueFamilyIndices))
{
}

VulkanBufferManager::~VulkanBufferManager()
{
    Shutdown();
}

void VulkanBufferManager::CreateMemoryAllocator(const VkInstance &Instance, const VkDevice& LogicalDevice, const VkPhysicalDevice& PhysicalDevice)
{
    m_Impl->CreateMemoryAllocator(Instance, LogicalDevice, PhysicalDevice);
}

void VulkanBufferManager::CreateSwapChain(const VkSurfaceFormatKHR &PreferredFormat, const VkPresentModeKHR &PreferredMode, const VkExtent2D &PreferredExtent, const VkSurfaceCapabilitiesKHR &Capabilities)
{
    m_Impl->CreateSwapChain(PreferredFormat, PreferredMode, PreferredExtent, Capabilities);
}

void VulkanBufferManager::CreateFrameBuffers(const VkRenderPass &RenderPass, const VkExtent2D &Extent)
{
    m_Impl->CreateFrameBuffers(RenderPass, Extent);
}

void VulkanBufferManager::CreateVertexBuffers(const VkQueue &Queue, const std::uint32_t QueueFamilyIndex)
{
    m_Impl->CreateVertexBuffers(Queue, QueueFamilyIndex);
}

void VulkanBufferManager::CreateIndexBuffers(const VkQueue &Queue, const std::uint32_t QueueFamilyIndex)
{
    m_Impl->CreateIndexBuffers(Queue, QueueFamilyIndex);
}

void VulkanBufferManager::CreateUniformBuffers()
{
    m_Impl->CreateUniformBuffers();
}

void VulkanBufferManager::UpdateUniformBuffers(const std::uint32_t Frame, const VkExtent2D &SwapChainExtent)
{
    m_Impl->UpdateUniformBuffers(Frame, SwapChainExtent);
}

void VulkanBufferManager::CreateTextureImage(const std::string_view Path, const VkQueue &Queue, const std::uint32_t QueueFamilyIndex, VkImageView& ImageView, VkSampler& Sampler)
{
    m_Impl->CreateTextureImage(Path, Queue, QueueFamilyIndex, ImageView, Sampler);
}

void VulkanBufferManager::CreateDepthResources(const VkFormat &Format, const VkExtent2D &Extent, const VkQueue &Queue, const std::uint32_t QueueFamilyIndex)
{
    m_Impl->CreateDepthResources(Format, Extent, Queue, QueueFamilyIndex);
}

void VulkanBufferManager::LoadScene(const std::string_view Path)
{
    m_Impl->LoadScene(Path);
}

void VulkanBufferManager::DestroyResources()
{
    m_Impl->DestroyResources();
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

const std::vector<VkImage> &VulkanBufferManager::GetSwapChainImages() const
{
    return m_Impl->GetSwapChainImages();
}

const std::vector<VkFramebuffer> &VulkanBufferManager::GetFrameBuffers() const
{
    return m_Impl->GetFrameBuffers();
}

const std::vector<VkBuffer> &VulkanBufferManager::GetVertexBuffers() const
{
    return m_Impl->GetVertexBuffers();
}

const std::vector<VkBuffer> &VulkanBufferManager::GetIndexBuffers() const
{
    return m_Impl->GetIndexBuffers();
}

const std::vector<VkBuffer> &VulkanBufferManager::GetUniformBuffers() const
{
    return m_Impl->GetUniformBuffers();
}

const std::vector<void *> &VulkanBufferManager::GetUniformBuffersData() const
{
    return m_Impl->GetUniformBuffersData();
}

const std::vector<Vertex> &VulkanBufferManager::GetVertices() const
{
    return m_Impl->GetVertices();
}

const std::vector<std::uint32_t> &VulkanBufferManager::GetIndices() const
{
    return m_Impl->GetIndices();
}

std::uint32_t VulkanBufferManager::GetIndicesCount() const
{
    return m_Impl->GetIndicesCount();
}