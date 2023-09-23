// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "Managers/VulkanBufferManager.h"
#include "Managers/VulkanRenderSubsystem.h"
#include "Utils/RenderCoreHelpers.h"
#include <chrono>
#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>
#include <boost/log/trivial.hpp>

#ifndef VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#endif
#include <vk_mem_alloc.h>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include <stb_image.h>

using namespace RenderCore;

bool VulkanImageAllocation::IsValid() const
{
    return Image != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE;
}

void VulkanImageAllocation::DestroyResources(const VmaAllocator &Allocator)
{
    if (Image != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE)
    {
        vmaDestroyImage(Allocator, Image, Allocation);
        Image = VK_NULL_HANDLE;
        Allocation = VK_NULL_HANDLE;
    }

    const VkDevice &VulkanLogicalDevice = VulkanRenderSubsystem::Get().GetDevice();

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

bool VulkanBufferAllocation::IsValid() const
{
    return Buffer != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE;
}

void VulkanBufferAllocation::DestroyResources(const VmaAllocator &Allocator)
{
    if (Buffer != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(Allocator, Buffer, Allocation);
        Allocation = VK_NULL_HANDLE;
        Buffer = VK_NULL_HANDLE;
    }
}

bool VulkanObjectAllocation::IsValid() const
{
    return TextureImage.IsValid() && VertexBuffer.IsValid() && IndexBuffer.IsValid() && IndicesCount != 0u;
}

void VulkanObjectAllocation::DestroyResources(const VmaAllocator &Allocator)
{
    VertexBuffer.DestroyResources(Allocator);
    IndexBuffer.DestroyResources(Allocator);
    TextureImage.DestroyResources(Allocator);
    IndicesCount = 0u;
}

VulkanBufferManager::VulkanBufferManager()
    : m_Allocator(VK_NULL_HANDLE)
    , m_SwapChain(VK_NULL_HANDLE)
    , m_OldSwapChain(VK_NULL_HANDLE)
    , m_SwapChainImages({})
    , m_DepthImage()
    , m_FrameBuffers({})
    , m_Objects({})
{
}

VulkanBufferManager::~VulkanBufferManager()
{
    if (!IsInitialized())
    {
        return;
    }

    Shutdown();
}

void VulkanBufferManager::CreateMemoryAllocator()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan memory allocator";

    const VmaVulkanFunctions VulkanFunctions{
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr};

    const VmaAllocatorCreateInfo AllocatorInfo{
        .flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT,
        .physicalDevice = VulkanRenderSubsystem::Get().GetPhysicalDevice(),
        .device = VulkanRenderSubsystem::Get().GetDevice(),
        .preferredLargeHeapBlockSize = 0u /*Default: 256 MiB*/,
        .pAllocationCallbacks = nullptr,
        .pDeviceMemoryCallbacks = nullptr,
        .pHeapSizeLimit = nullptr,
        .pVulkanFunctions = &VulkanFunctions,
        .instance = VulkanRenderSubsystem::Get().GetInstance(),
        .vulkanApiVersion = VK_API_VERSION_1_0,
        .pTypeExternalMemoryHandleTypes = nullptr};

    RENDERCORE_CHECK_VULKAN_RESULT(vmaCreateAllocator(&AllocatorInfo, &m_Allocator));
}

void VulkanBufferManager::CreateSwapChain(const bool bRecreate)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan swap chain";

    const VulkanDeviceProperties &Properties = VulkanRenderSubsystem::Get().GetDeviceProperties();

    const std::vector<std::uint32_t> QueueFamilyIndices = VulkanRenderSubsystem::Get().GetQueueFamilyIndices_u32();
    const std::uint32_t QueueFamilyIndicesCount = static_cast<std::uint32_t>(QueueFamilyIndices.size());

    m_OldSwapChain = m_SwapChain;

    const VkSwapchainCreateInfoKHR SwapChainCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = VulkanRenderSubsystem::Get().GetSurface(),
        .minImageCount = VulkanRenderSubsystem::Get().GetMinImageCount(),
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
        .oldSwapchain = m_OldSwapChain};

    const VkDevice &VulkanLogicalDevice = VulkanRenderSubsystem::Get().GetDevice();

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateSwapchainKHR(VulkanLogicalDevice, &SwapChainCreateInfo, nullptr, &m_SwapChain));

    if (bRecreate && m_OldSwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(VulkanLogicalDevice, m_OldSwapChain, nullptr);
        m_OldSwapChain = VK_NULL_HANDLE;
    }

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
}

void VulkanBufferManager::CreateFrameBuffers(const VkRenderPass &RenderPass)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan frame buffers";

    if (RenderPass == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan render pass is invalid.");
    }

    const VkDevice &VulkanLogicalDevice = VulkanRenderSubsystem::Get().GetDevice();
    const VulkanDeviceProperties &Properties = VulkanRenderSubsystem::Get().GetDeviceProperties();

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

void VulkanBufferManager::CreateVertexBuffers(VulkanObjectAllocation &Object, const std::vector<Vertex> &Vertices) const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan vertex buffers";

    if (m_Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    const VkQueue &TransferQueue = VulkanRenderSubsystem::Get().GetQueueFromType(VulkanQueueType::Transfer);
    const std::uint8_t TransferQueueFamilyIndex = VulkanRenderSubsystem::Get().GetQueueFamilyIndexFromType(VulkanQueueType::Transfer);

    constexpr VkBufferUsageFlags SourceUsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    constexpr VkBufferUsageFlags DestinationUsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = 0u;

    const VkDeviceSize BufferSize = Vertices.size() * sizeof(Vertex);

    VkBuffer StagingBuffer;
    VmaAllocation StagingBufferMemory;
    const VmaAllocationInfo StagingInfo = CreateBuffer(BufferSize, SourceUsageFlags, SourceMemoryPropertyFlags, StagingBuffer, StagingBufferMemory);

    std::memcpy(StagingInfo.pMappedData, Vertices.data(), static_cast<std::size_t>(BufferSize));

    CreateBuffer(BufferSize, DestinationUsageFlags, DestinationMemoryPropertyFlags, Object.VertexBuffer.Buffer, Object.VertexBuffer.Allocation);
    CopyBuffer(StagingBuffer, Object.VertexBuffer.Buffer, BufferSize, TransferQueue, TransferQueueFamilyIndex);

    vmaDestroyBuffer(m_Allocator, StagingBuffer, StagingBufferMemory);
}

void VulkanBufferManager::CreateIndexBuffers(VulkanObjectAllocation &Object, const std::vector<std::uint32_t> &Indices) const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan index buffers";

    if (m_Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    const VkQueue &TransferQueue = VulkanRenderSubsystem::Get().GetQueueFromType(VulkanQueueType::Transfer);
    const std::uint8_t TransferQueueFamilyIndex = VulkanRenderSubsystem::Get().GetQueueFamilyIndexFromType(VulkanQueueType::Transfer);

    constexpr VkBufferUsageFlags SourceUsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    constexpr VkBufferUsageFlags DestinationUsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = 0u;

    const VkDeviceSize BufferSize = Indices.size() * sizeof(std::uint32_t);

    VkBuffer StagingBuffer;
    VmaAllocation StagingBufferMemory;
    const VmaAllocationInfo StagingInfo = CreateBuffer(BufferSize, SourceUsageFlags, SourceMemoryPropertyFlags, StagingBuffer, StagingBufferMemory);

    std::memcpy(StagingInfo.pMappedData, Indices.data(), static_cast<std::size_t>(BufferSize));
    RENDERCORE_CHECK_VULKAN_RESULT(vmaFlushAllocation(m_Allocator, StagingBufferMemory, 0u, BufferSize));

    CreateBuffer(BufferSize, DestinationUsageFlags, DestinationMemoryPropertyFlags, Object.IndexBuffer.Buffer, Object.IndexBuffer.Allocation);
    CopyBuffer(StagingBuffer, Object.IndexBuffer.Buffer, BufferSize, TransferQueue, TransferQueueFamilyIndex);

    vmaDestroyBuffer(m_Allocator, StagingBuffer, StagingBufferMemory);
}

void VulkanBufferManager::CreateDepthResources()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan depth resources";

    const VulkanDeviceProperties &Properties = VulkanRenderSubsystem::Get().GetDeviceProperties();

    const VkQueue &GraphicsQueue = VulkanRenderSubsystem::Get().GetQueueFromType(VulkanQueueType::Graphics);
    const std::uint8_t GraphicsQueueFamilyIndex = VulkanRenderSubsystem::Get().GetQueueFamilyIndexFromType(VulkanQueueType::Graphics);

    constexpr VkImageTiling Tiling = VK_IMAGE_TILING_OPTIMAL;
    constexpr VkImageUsageFlagBits Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    constexpr VkMemoryPropertyFlags MemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    constexpr VkImageAspectFlagBits Aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
    constexpr VkImageLayout InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    constexpr VkImageLayout DestinationLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    CreateImage(Properties.DepthFormat, Properties.Extent, Tiling, Usage, MemoryPropertyFlags, m_DepthImage.Image, m_DepthImage.Allocation);
    CreateImageView(m_DepthImage.Image, Properties.DepthFormat, Aspect, m_DepthImage.View);
    MoveImageLayout(m_DepthImage.Image, Properties.DepthFormat, InitialLayout, DestinationLayout, GraphicsQueue, GraphicsQueueFamilyIndex);
}

std::uint64_t VulkanBufferManager::LoadObject(const std::string_view ModelPath, const std::string_view TexturePath)
{
    Assimp::Importer Importer;
    const aiScene *const Scene = Importer.ReadFile(ModelPath.data(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

    if (!Scene || Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !Scene->mRootNode)
    {
        throw std::runtime_error("Assimp error: " + std::string(Importer.GetErrorString()));
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loaded model from path: '" << ModelPath << "'";

    static std::atomic<std::uint64_t> ObjectID = 0u;
    const std::uint64_t NewID = ObjectID.fetch_add(1u);

    std::vector<Vertex> Vertices;
    std::vector<std::uint32_t> Indices;

    const std::vector<aiMesh *> Meshes(Scene->mMeshes, Scene->mMeshes + Scene->mNumMeshes);
    for (const aiMesh *MeshIter : Meshes)
    {
        for (std::uint32_t Iterator = 0u; Iterator < MeshIter->mNumVertices; ++Iterator)
        {
            const aiVector3D Position = MeshIter->mVertices[Iterator];
            aiVector3D TextureCoord(0.f);

            if (MeshIter->mTextureCoords != nullptr)
            {
                TextureCoord = MeshIter->mTextureCoords[0][Iterator];
            }

            Vertices.emplace_back(
                glm::vec3(Position.x, Position.y, Position.z),
                glm::vec3(1.f, 1.f, 1.f),
                glm::vec2(TextureCoord.x, TextureCoord.y));
        }

        const std::vector<aiFace> Faces(MeshIter->mFaces, MeshIter->mFaces + MeshIter->mNumFaces);
        for (const aiFace &Face : Faces)
        {
            for (std::uint32_t FaceIterator = 0u; FaceIterator < Face.mNumIndices; ++FaceIterator)
            {
                Indices.push_back(Face.mIndices[FaceIterator]);
            }
        }
    }

    VulkanObjectAllocation NewObject{.IndicesCount = static_cast<std::uint32_t>(Indices.size())};
    CreateVertexBuffers(NewObject, Vertices);
    CreateIndexBuffers(NewObject, Indices);
    LoadTexture(NewObject, TexturePath);

    m_Objects.emplace(NewID, NewObject);

    return NewID;
}

void VulkanBufferManager::LoadTexture(VulkanObjectAllocation &Object, const std::string_view TexturePath) const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan texture image";

    if (m_Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    std::int32_t Width;
    std::int32_t Height;
    std::int32_t Channels;

    std::string UsedTexturePath = TexturePath.data();
    if (!std::filesystem::exists(UsedTexturePath))
    {
        UsedTexturePath = EMPTY_TEX;
    }

    stbi_uc *const ImagePixels = stbi_load(UsedTexturePath.c_str(), &Width, &Height, &Channels, STBI_rgb_alpha);
    const VkDeviceSize AllocationSize = Width * Height * 4u;

    if (!ImagePixels)
    {
        throw std::runtime_error("STB Image is invalid. Path: " + std::string(TexturePath));
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loaded image from path: '" << TexturePath << "'";

    constexpr VkBufferUsageFlags SourceUsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    constexpr VkBufferUsageFlags DestinationUsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    constexpr VkImageTiling Tiling = VK_IMAGE_TILING_OPTIMAL;

    VkBuffer StagingBuffer;
    VmaAllocation StagingBufferMemory;
    const VmaAllocationInfo StagingInfo = CreateBuffer(AllocationSize, SourceUsageFlags, SourceMemoryPropertyFlags, StagingBuffer, StagingBufferMemory);

    std::memcpy(StagingInfo.pMappedData, ImagePixels, static_cast<std::size_t>(AllocationSize));

    stbi_image_free(ImagePixels);

    constexpr VkFormat DepthImageFormat = VK_FORMAT_R8G8B8A8_SRGB;
    constexpr VkImageLayout InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    constexpr VkImageLayout MiddleLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    constexpr VkImageLayout DestinationLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    const VkExtent2D Extent{
        .width = static_cast<std::uint32_t>(Width),
        .height = static_cast<std::uint32_t>(Height)};

    const VkQueue &GraphicsQueue = VulkanRenderSubsystem::Get().GetQueueFromType(VulkanQueueType::Graphics);
    const std::uint8_t GraphicsQueueFamilyIndex = VulkanRenderSubsystem::Get().GetQueueFamilyIndexFromType(VulkanQueueType::Graphics);

    CreateImage(DepthImageFormat, Extent, Tiling, DestinationUsageFlags, DestinationMemoryPropertyFlags, Object.TextureImage.Image, Object.TextureImage.Allocation);
    MoveImageLayout(Object.TextureImage.Image, DepthImageFormat, InitialLayout, MiddleLayout, GraphicsQueue, GraphicsQueueFamilyIndex);
    CopyBufferToImage(StagingBuffer, Object.TextureImage.Image, Extent, GraphicsQueue, GraphicsQueueFamilyIndex);
    MoveImageLayout(Object.TextureImage.Image, DepthImageFormat, MiddleLayout, DestinationLayout, GraphicsQueue, GraphicsQueueFamilyIndex);

    CreateTextureImageView(Object.TextureImage);
    CreateTextureSampler(Object.TextureImage);
    vmaDestroyBuffer(m_Allocator, StagingBuffer, StagingBufferMemory);
}

const bool VulkanBufferManager::GetObjectTexture(const std::uint64_t ObjectID, VulkanTextureData &TextureData) const
{
    if (!m_Objects.contains(ObjectID))
    {
        return false;
    }

    const VulkanObjectAllocation &Object = m_Objects.at(ObjectID);

    TextureData.ImageView = Object.TextureImage.View;
    TextureData.Sampler = Object.TextureImage.Sampler;

    return true;
}

void VulkanBufferManager::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down Vulkan buffer manager";

    const VkDevice &VulkanLogicalDevice = VulkanRenderSubsystem::Get().GetDevice();

    if (m_SwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(VulkanLogicalDevice, m_SwapChain, nullptr);
        m_SwapChain = VK_NULL_HANDLE;
    }

    if (m_OldSwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(VulkanLogicalDevice, m_OldSwapChain, nullptr);
        m_OldSwapChain = VK_NULL_HANDLE;
    }

    DestroyResources(true);

    vmaDestroyAllocator(m_Allocator);
    m_Allocator = VK_NULL_HANDLE;
}

void VulkanBufferManager::DestroyResources(const bool bClearScene)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying Vulkan buffer manager resources";

    const VkDevice &VulkanLogicalDevice = VulkanRenderSubsystem::Get().GetDevice();

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

    if (bClearScene)
    {
        for (auto &[ObjectID, ObjectIter] : m_Objects)
        {
            ObjectIter.DestroyResources(m_Allocator);
        }
    }
}

bool VulkanBufferManager::IsInitialized() const
{
    return m_Allocator != VK_NULL_HANDLE;
}

const VkSwapchainKHR &VulkanBufferManager::GetSwapChain() const
{
    return m_SwapChain;
}

const std::vector<VkImage> VulkanBufferManager::GetSwapChainImages() const
{
    std::vector<VkImage> SwapChainImages;
    for (const VulkanImageAllocation &ImageIter : m_SwapChainImages)
    {
        SwapChainImages.push_back(ImageIter.Image);
    }

    return SwapChainImages;
}

const std::vector<VkFramebuffer> &VulkanBufferManager::GetFrameBuffers() const
{
    return m_FrameBuffers;
}

const VkBuffer VulkanBufferManager::GetVertexBuffer(const std::uint64_t ObjectID) const
{
    return m_Objects.at(ObjectID).VertexBuffer.Buffer;
}

const VkBuffer VulkanBufferManager::GetIndexBuffer(const std::uint64_t ObjectID) const
{
    return m_Objects.at(ObjectID).IndexBuffer.Buffer;
}

const std::uint32_t VulkanBufferManager::GetIndicesCount(const std::uint64_t ObjectID) const
{
    return static_cast<std::uint32_t>(m_Objects.at(ObjectID).IndicesCount);
}

VmaAllocationInfo VulkanBufferManager::CreateBuffer(const VkDeviceSize &Size, const VkBufferUsageFlags Usage, const VkMemoryPropertyFlags Flags, VkBuffer &Buffer, VmaAllocation &Allocation) const
{
    if (m_Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    const VkBufferCreateInfo BufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = Size,
        .usage = Usage};

    const VmaAllocationCreateInfo AllocationCreateInfo{
        .flags = Flags,
        .usage = VMA_MEMORY_USAGE_AUTO};

    VmaAllocationInfo MemoryAllocationInfo;
    RENDERCORE_CHECK_VULKAN_RESULT(vmaCreateBuffer(m_Allocator, &BufferCreateInfo, &AllocationCreateInfo, &Buffer, &Allocation, &MemoryAllocationInfo));

    return MemoryAllocationInfo;
}

void VulkanBufferManager::CopyBuffer(const VkBuffer &Source, const VkBuffer &Destination, const VkDeviceSize &Size, const VkQueue &Queue, const std::uint8_t QueueFamilyIndex) const
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

void VulkanBufferManager::CreateImage(const VkFormat &ImageFormat, const VkExtent2D &Extent, const VkImageTiling &Tiling, const VkImageUsageFlags Usage, const VkMemoryPropertyFlags Flags, VkImage &Image, VmaAllocation &Allocation) const
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
        .flags = Flags,
        .usage = VMA_MEMORY_USAGE_AUTO};

    VmaAllocationInfo AllocationInfo;
    RENDERCORE_CHECK_VULKAN_RESULT(vmaCreateImage(m_Allocator, &ImageViewCreateInfo, &ImageAllocationCreateInfo, &Image, &Allocation, &AllocationInfo));
}

void VulkanBufferManager::CreateImageView(const VkImage &Image, const VkFormat &Format, const VkImageAspectFlags &AspectFlags, VkImageView &ImageView) const
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
        .subresourceRange = {.aspectMask = AspectFlags, .baseMipLevel = 0u, .levelCount = 1u, .baseArrayLayer = 0u, .layerCount = 1u}};

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateImageView(VulkanRenderSubsystem::Get().GetDevice(), &ImageViewCreateInfo, nullptr, &ImageView));
}

void VulkanBufferManager::CreateTextureSampler(VulkanImageAllocation &Allocation) const
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
        .maxLod = FLT_MAX,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE};

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateSampler(VulkanRenderSubsystem::Get().GetDevice(), &SamplerCreateInfo, nullptr, &Allocation.Sampler));
}

void VulkanBufferManager::CopyBufferToImage(const VkBuffer &Source, const VkImage &Destination, const VkExtent2D &Extent, const VkQueue &Queue, const std::uint32_t QueueFamilyIndex) const
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

void VulkanBufferManager::MoveImageLayout(const VkImage &Image, const VkFormat &Format, const VkImageLayout &OldLayout, const VkImageLayout &NewLayout, const VkQueue &Queue, const std::uint8_t QueueFamilyIndex) const
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

void VulkanBufferManager::CreateTextureImageView(VulkanImageAllocation &Allocation) const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan texture image views";
    CreateImageView(Allocation.Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, Allocation.View);
}

void VulkanBufferManager::CreateSwapChainImageViews(const VkFormat &ImageFormat)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan swap chain image views";
    for (VulkanImageAllocation &ImageIter : m_SwapChainImages)
    {
        CreateImageView(ImageIter.Image, ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, ImageIter.View);
    }
}