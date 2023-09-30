// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

#include "VulkanRenderCore.h"
#include "Managers/VulkanBufferManager.h"
#include "Managers/VulkanDeviceManager.h"
#include "Managers/VulkanPipelineManager.h"
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
#include <ranges>
#include <stb_image.h>

using namespace RenderCore;

VulkanBufferManager VulkanBufferManager::g_Instance{};
VmaAllocator        VulkanBufferManager::g_Allocator(VK_NULL_HANDLE);

bool VulkanImageAllocation::IsValid() const
{
    return Image != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE;
}

void VulkanImageAllocation::DestroyResources()
{
    if (Image != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE)
    {
        vmaDestroyImage(VulkanBufferManager::g_Allocator, Image, Allocation);
        Image      = VK_NULL_HANDLE;
        Allocation = VK_NULL_HANDLE;
    }

    const VkDevice& VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

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

void VulkanBufferAllocation::DestroyResources()
{
    if (Buffer != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE)
    {
        if (Allocation->GetMappedData() != nullptr)
        {
            vmaUnmapMemory(VulkanBufferManager::g_Allocator, Allocation);
        }

        vmaDestroyBuffer(VulkanBufferManager::g_Allocator, Buffer, Allocation);
        Allocation = VK_NULL_HANDLE;
        Buffer     = VK_NULL_HANDLE;
    }
}

bool VulkanObjectAllocation::IsValid() const
{
    return TextureImage.IsValid() && VertexBuffer.IsValid() && IndexBuffer.IsValid() && IndicesCount != 0u;
}

void VulkanObjectAllocation::DestroyResources()
{
    VertexBuffer.DestroyResources();
    IndexBuffer.DestroyResources();
    TextureImage.DestroyResources();
    IndicesCount = 0u;
}

VulkanBufferManager::VulkanBufferManager()
    : m_SwapChain(VK_NULL_HANDLE)
  , m_OldSwapChain(VK_NULL_HANDLE)
  , m_SwapChainExtent({0u, 0u})
  , m_SwapChainImages({})
  , m_DepthImage()
  , m_FrameBuffers({})
  , m_Objects({})
  , m_ObjectIDCounter(0u)
{
}

VulkanBufferManager::~VulkanBufferManager()
{
    Shutdown();
}

VulkanBufferManager& VulkanBufferManager::Get()
{
    return g_Instance;
}

void VulkanBufferManager::CreateMemoryAllocator()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan memory allocator";

    const VmaVulkanFunctions VulkanFunctions{.vkGetInstanceProcAddr = vkGetInstanceProcAddr, .vkGetDeviceProcAddr = vkGetDeviceProcAddr};

    const VmaAllocatorCreateInfo AllocatorInfo{
        .flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT,
        .physicalDevice = VulkanDeviceManager::Get().GetPhysicalDevice(),
        .device = VulkanDeviceManager::Get().GetLogicalDevice(),
        .preferredLargeHeapBlockSize = 0u /*Default: 256 MiB*/,
        .pAllocationCallbacks = nullptr,
        .pDeviceMemoryCallbacks = nullptr,
        .pHeapSizeLimit = nullptr,
        .pVulkanFunctions = &VulkanFunctions,
        .instance = VulkanRenderCore::Get().GetInstance(),
        .vulkanApiVersion = VK_API_VERSION_1_0,
        .pTypeExternalMemoryHandleTypes = nullptr
    };

    RENDERCORE_CHECK_VULKAN_RESULT(vmaCreateAllocator(&AllocatorInfo, &g_Allocator));
}

void VulkanBufferManager::CreateSwapChain()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan swap chain";

    // ReSharper disable once CppUseStructuredBinding
    const VulkanDeviceProperties& Properties = VulkanDeviceManager::Get().GetDeviceProperties();

    const std::vector<std::uint32_t> QueueFamilyIndices      = VulkanDeviceManager::Get().GetUniqueQueueFamilyIndicesU32();
    const std::uint32_t              QueueFamilyIndicesCount = QueueFamilyIndices.size();

    m_OldSwapChain    = m_SwapChain;
    m_SwapChainExtent = Properties.Extent;

    const VkSwapchainCreateInfoKHR SwapChainCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = VulkanRenderCore::Get().GetSurface(),
        .minImageCount = VulkanDeviceManager::Get().GetMinImageCount(),
        .imageFormat = Properties.Format.format,
        .imageColorSpace = Properties.Format.colorSpace,
        .imageExtent = m_SwapChainExtent,
        .imageArrayLayers = 1u,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = QueueFamilyIndicesCount > 1u ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = QueueFamilyIndicesCount,
        .pQueueFamilyIndices = QueueFamilyIndices.data(),
        .preTransform = Properties.Capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = Properties.Mode,
        .clipped = VK_TRUE,
        .oldSwapchain = m_OldSwapChain
    };

    const VkDevice& VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateSwapchainKHR(VulkanLogicalDevice, &SwapChainCreateInfo, nullptr, &m_SwapChain));

    if (m_OldSwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(VulkanLogicalDevice, m_OldSwapChain, nullptr);
        m_OldSwapChain = VK_NULL_HANDLE;
    }

    std::uint32_t Count = 0u;
    RENDERCORE_CHECK_VULKAN_RESULT(vkGetSwapchainImagesKHR(VulkanLogicalDevice, m_SwapChain, &Count, nullptr));

    std::vector<VkImage> SwapChainImages(Count, VK_NULL_HANDLE);
    RENDERCORE_CHECK_VULKAN_RESULT(vkGetSwapchainImagesKHR(VulkanLogicalDevice, m_SwapChain, &Count, SwapChainImages.data()));

    m_SwapChainImages.resize(Count, VulkanImageAllocation());
    for (std::uint32_t Iterator = 0u; Iterator < Count; ++Iterator)
    {
        m_SwapChainImages[Iterator].Image = SwapChainImages[Iterator];
    }

    CreateSwapChainImageViews(Properties.Format.format);
}

void VulkanBufferManager::CreateFrameBuffers()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan frame buffers";

    const VkDevice& VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

    m_FrameBuffers.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);
    for (std::uint32_t Iterator = 0u; Iterator < static_cast<std::uint32_t>(m_FrameBuffers.size()); ++Iterator)
    {
        const std::array Attachments{m_SwapChainImages[Iterator].View, m_DepthImage.View};

        const VkFramebufferCreateInfo FrameBufferCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = VulkanPipelineManager::Get().GetRenderPass(),
            .attachmentCount = static_cast<std::uint32_t>(Attachments.size()),
            .pAttachments = Attachments.data(),
            .width = m_SwapChainExtent.width,
            .height = m_SwapChainExtent.height,
            .layers = 1u
        };

        RENDERCORE_CHECK_VULKAN_RESULT(vkCreateFramebuffer(VulkanLogicalDevice, &FrameBufferCreateInfo, nullptr, &m_FrameBuffers[Iterator]));
    }
}

void VulkanBufferManager::CreateDepthResources()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan depth resources";

    // ReSharper disable once CppUseStructuredBinding
    const VulkanDeviceProperties& Properties           = VulkanDeviceManager::Get().GetDeviceProperties();
    const auto&                   [FamilyIndex, Queue] = VulkanDeviceManager::Get().GetGraphicsQueue();

    constexpr VkImageTiling         Tiling              = VK_IMAGE_TILING_OPTIMAL;
    constexpr VkImageUsageFlagBits  Usage               = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    constexpr VkMemoryPropertyFlags MemoryPropertyFlags = 0u;
    constexpr VkImageAspectFlagBits Aspect              = VK_IMAGE_ASPECT_DEPTH_BIT;
    constexpr VkImageLayout         InitialLayout       = VK_IMAGE_LAYOUT_UNDEFINED;
    constexpr VkImageLayout         DestinationLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    CreateImage(Properties.DepthFormat, m_SwapChainExtent, Tiling, Usage, MemoryPropertyFlags, m_DepthImage.Image, m_DepthImage.Allocation);
    CreateImageView(m_DepthImage.Image, Properties.DepthFormat, Aspect, m_DepthImage.View);
    MoveImageLayout(m_DepthImage.Image, Properties.DepthFormat, InitialLayout, DestinationLayout, Queue, FamilyIndex);
}

void VulkanBufferManager::CreateVertexBuffers(VulkanObjectAllocation& Object, const std::vector<Vertex>& Vertices)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan vertex buffers";

    if (g_Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    const auto& [FamilyIndex, Queue] = VulkanDeviceManager::Get().GetTransferQueue();

    constexpr VkBufferUsageFlags    SourceUsageFlags          = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    constexpr VkBufferUsageFlags    DestinationUsageFlags          = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = 0u;

    const VkDeviceSize BufferSize = Vertices.size() * sizeof(Vertex);

    VkBuffer      StagingBuffer;
    VmaAllocation StagingBufferMemory;
    // ReSharper disable once CppUseStructuredBinding
    const VmaAllocationInfo StagingInfo = CreateBuffer(BufferSize, SourceUsageFlags, SourceMemoryPropertyFlags, StagingBuffer, StagingBufferMemory);

    std::memcpy(StagingInfo.pMappedData, Vertices.data(), BufferSize);

    CreateBuffer(BufferSize, DestinationUsageFlags, DestinationMemoryPropertyFlags, Object.VertexBuffer.Buffer, Object.VertexBuffer.Allocation);
    CopyBuffer(StagingBuffer, Object.VertexBuffer.Buffer, BufferSize, Queue, FamilyIndex);

    vmaDestroyBuffer(g_Allocator, StagingBuffer, StagingBufferMemory);
}

void VulkanBufferManager::CreateIndexBuffers(VulkanObjectAllocation& Object, const std::vector<std::uint32_t>& Indices)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan index buffers";

    if (g_Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    const auto& [FamilyIndex, Queue] = VulkanDeviceManager::Get().GetTransferQueue();

    constexpr VkBufferUsageFlags    SourceUsageFlags          = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    constexpr VkBufferUsageFlags    DestinationUsageFlags          = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = 0u;

    const VkDeviceSize BufferSize = Indices.size() * sizeof(std::uint32_t);

    VkBuffer      StagingBuffer;
    VmaAllocation StagingBufferMemory;

    // ReSharper disable once CppUseStructuredBinding
    const VmaAllocationInfo StagingInfo = CreateBuffer(BufferSize, SourceUsageFlags, SourceMemoryPropertyFlags, StagingBuffer, StagingBufferMemory);

    std::memcpy(StagingInfo.pMappedData, Indices.data(), BufferSize);
    RENDERCORE_CHECK_VULKAN_RESULT(vmaFlushAllocation(g_Allocator, StagingBufferMemory, 0u, BufferSize));

    CreateBuffer(BufferSize, DestinationUsageFlags, DestinationMemoryPropertyFlags, Object.IndexBuffer.Buffer, Object.IndexBuffer.Allocation);
    CopyBuffer(StagingBuffer, Object.IndexBuffer.Buffer, BufferSize, Queue, FamilyIndex);

    vmaDestroyBuffer(g_Allocator, StagingBuffer, StagingBufferMemory);
}

std::uint64_t VulkanBufferManager::LoadObject(const std::string_view ModelPath, const std::string_view TexturePath)
{
    Assimp::Importer     Importer;
    const aiScene* const Scene = Importer.ReadFile(ModelPath.data(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

    if (!Scene || Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !Scene->mRootNode)
    {
        throw std::runtime_error("Assimp error: " + std::string(Importer.GetErrorString()));
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loaded model from path: '" << ModelPath << "'";

    const std::uint64_t NewID = m_ObjectIDCounter.fetch_add(1u);

    std::vector<Vertex>        Vertices;
    std::vector<std::uint32_t> Indices;

    for (const std::vector Meshes(Scene->mMeshes, Scene->mMeshes + Scene->mNumMeshes);
         const aiMesh*     MeshIter : Meshes)
    {
        for (std::uint32_t Iterator = 0u; Iterator < MeshIter->mNumVertices; ++Iterator)
        {
            const aiVector3D Position = MeshIter->mVertices[Iterator];
            aiVector3D       TextureCoord(0.f);

            if (MeshIter->mTextureCoords != nullptr)
            {
                TextureCoord = MeshIter->mTextureCoords[0][Iterator];
            }

            Vertices.emplace_back(glm::vec3(Position.x, Position.y, Position.z), glm::vec3(1.f, 1.f, 1.f), glm::vec2(TextureCoord.x, TextureCoord.y));
        }

        for (const std::vector Faces(MeshIter->mFaces, MeshIter->mFaces + MeshIter->mNumFaces);
             const aiFace&     Face : Faces)
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

void VulkanBufferManager::UnLoadObject(const std::uint64_t ObjectID)
{
    if (!m_Objects.contains(ObjectID))
    {
        return;
    }

    m_Objects.at(ObjectID).DestroyResources();
    m_Objects.erase(ObjectID);
}

VulkanImageAllocation VulkanBufferManager::AllocateTexture(const unsigned char* Data, const std::uint32_t Width, const std::uint32_t Height, const std::size_t AllocationSize)
{
    VulkanImageAllocation ImageAllocation;

    constexpr VkBufferUsageFlags    SourceUsageFlags          = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    constexpr VkBufferUsageFlags    DestinationUsageFlags          = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    constexpr VkImageTiling Tiling = VK_IMAGE_TILING_OPTIMAL;

    VkBuffer      StagingBuffer;
    VmaAllocation StagingBufferMemory;

    // ReSharper disable once CppUseStructuredBinding
    const VmaAllocationInfo StagingInfo = CreateBuffer(AllocationSize, SourceUsageFlags, SourceMemoryPropertyFlags, StagingBuffer, StagingBufferMemory);

    std::memcpy(StagingInfo.pMappedData, Data, AllocationSize);

    constexpr VkFormat      ImageFormat       = VK_FORMAT_R8G8B8A8_SRGB;
    constexpr VkImageLayout InitialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    constexpr VkImageLayout MiddleLayout      = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    constexpr VkImageLayout DestinationLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    const VkExtent2D Extent{.width = Width, .height = Height};

    const auto& [FamilyIndex, Queue] = VulkanDeviceManager::Get().GetGraphicsQueue();

    CreateImage(ImageFormat, Extent, Tiling, DestinationUsageFlags, DestinationMemoryPropertyFlags, ImageAllocation.Image, ImageAllocation.Allocation);
    MoveImageLayout(ImageAllocation.Image, ImageFormat, InitialLayout, MiddleLayout, Queue, FamilyIndex);
    CopyBufferToImage(StagingBuffer, ImageAllocation.Image, Extent, Queue, FamilyIndex);
    MoveImageLayout(ImageAllocation.Image, ImageFormat, MiddleLayout, DestinationLayout, Queue, FamilyIndex);

    CreateTextureImageView(ImageAllocation);
    CreateTextureSampler(ImageAllocation);
    vmaDestroyBuffer(g_Allocator, StagingBuffer, StagingBufferMemory);

    return ImageAllocation;
}

void VulkanBufferManager::LoadTexture(VulkanObjectAllocation& Object, const std::string_view TexturePath)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan texture image";

    if (g_Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    std::int32_t Width    = -1;
    std::int32_t Height   = -1;
    std::int32_t Channels = -1;

    std::string UsedTexturePath = TexturePath.data();
    if (!std::filesystem::exists(UsedTexturePath))
    {
        UsedTexturePath = EMPTY_TEX;
    }

    const stbi_uc* const ImagePixels    = stbi_load(UsedTexturePath.c_str(), &Width, &Height, &Channels, STBI_rgb_alpha);
    const VkDeviceSize   AllocationSize = Width * Height * 4;

    if (!ImagePixels)
    {
        throw std::runtime_error("STB Image is invalid. Path: " + std::string(TexturePath));
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loaded image from path: '" << TexturePath << "'";

    Object.TextureImage = AllocateTexture(ImagePixels, static_cast<std::uint32_t>(Width), static_cast<std::uint32_t>(Height), AllocationSize);
}

VmaAllocationInfo VulkanBufferManager::CreateBuffer(const VkDeviceSize& Size, const VkBufferUsageFlags Usage, const VkMemoryPropertyFlags Flags, VkBuffer& Buffer, VmaAllocation& Allocation)
{
    if (g_Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    const VkBufferCreateInfo BufferCreateInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = Size, .usage = Usage};

    const VmaAllocationCreateInfo AllocationCreateInfo{.flags = Flags, .usage = VMA_MEMORY_USAGE_AUTO};

    VmaAllocationInfo MemoryAllocationInfo;
    RENDERCORE_CHECK_VULKAN_RESULT(vmaCreateBuffer(g_Allocator, &BufferCreateInfo, &AllocationCreateInfo, &Buffer, &Allocation, &MemoryAllocationInfo));

    return MemoryAllocationInfo;
}

void VulkanBufferManager::CopyBuffer(const VkBuffer& Source, const VkBuffer& Destination, const VkDeviceSize& Size, const VkQueue& Queue, const std::uint8_t QueueFamilyIndex)
{
    VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
    VkCommandPool   CommandPool   = VK_NULL_HANDLE;
    RenderCoreHelpers::InitializeSingleCommandQueue(CommandPool, CommandBuffer, QueueFamilyIndex);
    {
        const VkBufferCopy BufferCopy{.size = Size,};

        vkCmdCopyBuffer(CommandBuffer, Source, Destination, 1u, &BufferCopy);
    }
    RenderCoreHelpers::FinishSingleCommandQueue(Queue, CommandPool, CommandBuffer);
}

void VulkanBufferManager::CreateImage(const VkFormat&             ImageFormat,
                                      const VkExtent2D&           Extent,
                                      const VkImageTiling&        Tiling,
                                      const VkImageUsageFlags     Usage,
                                      const VkMemoryPropertyFlags Flags,
                                      VkImage&                    Image,
                                      VmaAllocation&              Allocation)
{
    if (g_Allocator == VK_NULL_HANDLE)
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
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    const VmaAllocationCreateInfo ImageAllocationCreateInfo{.flags = Flags, .usage = VMA_MEMORY_USAGE_AUTO};

    VmaAllocationInfo AllocationInfo;
    RENDERCORE_CHECK_VULKAN_RESULT(vmaCreateImage(g_Allocator, &ImageViewCreateInfo, &ImageAllocationCreateInfo, &Image, &Allocation, &AllocationInfo));
}

void VulkanBufferManager::CreateImageView(const VkImage& Image, const VkFormat& Format, const VkImageAspectFlags& AspectFlags, VkImageView& ImageView)
{
    const VkImageViewCreateInfo ImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = Image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = Format,
        .components = {.r = VK_COMPONENT_SWIZZLE_IDENTITY, .g = VK_COMPONENT_SWIZZLE_IDENTITY, .b = VK_COMPONENT_SWIZZLE_IDENTITY, .a = VK_COMPONENT_SWIZZLE_IDENTITY},
        .subresourceRange = {.aspectMask = AspectFlags, .baseMipLevel = 0u, .levelCount = 1u, .baseArrayLayer = 0u, .layerCount = 1u}
    };

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateImageView(VulkanDeviceManager::Get().GetLogicalDevice(), &ImageViewCreateInfo, nullptr, &ImageView));
}

void VulkanBufferManager::CreateTextureImageView(VulkanImageAllocation& Allocation)
{
    CreateImageView(Allocation.Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, Allocation.View);
}

void VulkanBufferManager::CreateTextureSampler(VulkanImageAllocation& Allocation)
{
    if (g_Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    VkPhysicalDeviceProperties DeviceProperties;
    vkGetPhysicalDeviceProperties(g_Allocator->GetPhysicalDevice(), &DeviceProperties);

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
        .unnormalizedCoordinates = VK_FALSE
    };

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateSampler(VulkanDeviceManager::Get().GetLogicalDevice(), &SamplerCreateInfo, nullptr, &Allocation.Sampler));
}

void VulkanBufferManager::CopyBufferToImage(const VkBuffer& Source, const VkImage& Destination, const VkExtent2D& Extent, const VkQueue& Queue, const std::uint32_t QueueFamilyIndex)
{
    VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
    VkCommandPool   CommandPool   = VK_NULL_HANDLE;
    RenderCoreHelpers::InitializeSingleCommandQueue(CommandPool, CommandBuffer, QueueFamilyIndex);
    {
        const VkBufferImageCopy BufferImageCopy{
            .bufferOffset = 0u,
            .bufferRowLength = 0u,
            .bufferImageHeight = 0u,
            .imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0u, .baseArrayLayer = 0u, .layerCount = 1u},
            .imageOffset = {.x = 0u, .y = 0u, .z = 0u},
            .imageExtent = {.width = Extent.width, .height = Extent.height, .depth = 1u}
        };

        vkCmdCopyBufferToImage(CommandBuffer, Source, Destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &BufferImageCopy);
    }
    RenderCoreHelpers::FinishSingleCommandQueue(Queue, CommandPool, CommandBuffer);
}

void VulkanBufferManager::MoveImageLayout(const VkImage&       Image,
                                          const VkFormat&      Format,
                                          const VkImageLayout& OldLayout,
                                          const VkImageLayout& NewLayout,
                                          const VkQueue&       Queue,
                                          const std::uint8_t   QueueFamilyIndex)
{
    VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
    VkCommandPool   CommandPool   = VK_NULL_HANDLE;
    RenderCoreHelpers::InitializeSingleCommandQueue(CommandPool, CommandBuffer, QueueFamilyIndex);
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
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0u, .levelCount = 1u, .baseArrayLayer = 0u, .layerCount = 1u}
        };

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

            SourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            DestinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            SourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
            DestinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            Barrier.srcAccessMask = 0;
            Barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            SourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            DestinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else
        {
            throw std::invalid_argument("Vulkan image layout transition is invalid");
        }

        vkCmdPipelineBarrier(CommandBuffer, SourceStage, DestinationStage, 0u, 0u, nullptr, 0u, nullptr, 1u, &Barrier);
    }
    RenderCoreHelpers::FinishSingleCommandQueue(Queue, CommandPool, CommandBuffer);
}

void VulkanBufferManager::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down Vulkan buffer manager";

    const VkDevice& VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

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

    vmaDestroyAllocator(g_Allocator);
    g_Allocator = VK_NULL_HANDLE;
}

void VulkanBufferManager::DestroyResources(const bool ClearScene)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying Vulkan buffer manager resources";

    const VkDevice& VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

    for (VulkanImageAllocation& ImageViewIter : m_SwapChainImages)
    {
        ImageViewIter.DestroyResources();
    }
    m_SwapChainImages.clear();

    for (VkFramebuffer& FrameBufferIter : m_FrameBuffers)
    {
        if (FrameBufferIter != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(VulkanLogicalDevice, FrameBufferIter, nullptr);
            FrameBufferIter = VK_NULL_HANDLE;
        }
    }
    m_FrameBuffers.clear();

    m_DepthImage.DestroyResources();

    if (ClearScene)
    {
        for (auto& ObjectIter : m_Objects | std::views::values)
        {
            ObjectIter.DestroyResources();
        }
    }
}

bool VulkanBufferManager::IsInitialized()
{
    return g_Allocator != VK_NULL_HANDLE;
}

const VkSwapchainKHR& VulkanBufferManager::GetSwapChain() const
{
    return m_SwapChain;
}

const VkExtent2D& VulkanBufferManager::GetSwapChainExtent() const
{
    return m_SwapChainExtent;
}

std::vector<VkImage> VulkanBufferManager::GetSwapChainImages() const
{
    std::vector<VkImage> SwapChainImages;
    for (const auto& [Image, View, Sampler, Allocation] : m_SwapChainImages)
    {
        SwapChainImages.push_back(Image);
    }

    return SwapChainImages;
}

const std::vector<VkFramebuffer>& VulkanBufferManager::GetFrameBuffers() const
{
    return m_FrameBuffers;
}

VkBuffer VulkanBufferManager::GetVertexBuffer(const std::uint64_t ObjectID) const
{
    if (!m_Objects.contains(ObjectID))
    {
        return VK_NULL_HANDLE;
    }

    return m_Objects.at(ObjectID).VertexBuffer.Buffer;
}

VkBuffer VulkanBufferManager::GetIndexBuffer(const std::uint64_t ObjectID) const
{
    if (!m_Objects.contains(ObjectID))
    {
        return VK_NULL_HANDLE;
    }

    return m_Objects.at(ObjectID).IndexBuffer.Buffer;
}

std::uint32_t VulkanBufferManager::GetIndicesCount(const std::uint64_t ObjectID) const
{
    if (!m_Objects.contains(ObjectID))
    {
        return 0u;
    }

    return m_Objects.at(ObjectID).IndicesCount;
}

std::vector<VulkanTextureData> VulkanBufferManager::GetAllocatedTextures() const
{
    std::vector<VulkanTextureData> Output;
    for (const auto& [TextureImage, VertexBuffer, IndexBuffer, IndicesCount] : m_Objects | std::views::values)
    {
        Output.emplace_back(TextureImage.View, TextureImage.Sampler);
    }
    return Output;
}

void VulkanBufferManager::CreateSwapChainImageViews(const VkFormat& ImageFormat)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan swap chain image views";
    for (auto& [Image, View, Sampler, Allocation] : m_SwapChainImages)
    {
        CreateImageView(Image, ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, View);
    }
}
