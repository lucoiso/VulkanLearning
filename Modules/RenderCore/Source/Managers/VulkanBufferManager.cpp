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

bool VulkanImageAllocation::IsValid() const
{
    return Image != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE;
}

void VulkanImageAllocation::DestroyResources()
{
    if (Image != VK_NULL_HANDLE && Allocation != VK_NULL_HANDLE)
    {
        vmaDestroyImage(VulkanBufferManager::Get().GetAllocator(), Image, Allocation);
        Image      = VK_NULL_HANDLE;
        Allocation = VK_NULL_HANDLE;
    }

    VkDevice const& VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

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
            vmaUnmapMemory(VulkanBufferManager::Get().GetAllocator(), Allocation);
        }

        vmaDestroyBuffer(VulkanBufferManager::Get().GetAllocator(), Buffer, Allocation);
        Allocation = VK_NULL_HANDLE;
        Buffer     = VK_NULL_HANDLE;
    }
}

bool VulkanObjectAllocation::IsValid() const
{
    return TextureImage.IsValid() && VertexBuffer.IsValid() && IndexBuffer.IsValid() && IndicesCount != 0U;
}

void VulkanObjectAllocation::DestroyResources()
{
    VertexBuffer.DestroyResources();
    IndexBuffer.DestroyResources();
    TextureImage.DestroyResources();
    IndicesCount = 0U;
}

VulkanBufferManager::VulkanBufferManager()
    : m_SwapChain(VK_NULL_HANDLE),
      m_OldSwapChain(VK_NULL_HANDLE),
      m_SwapChainExtent({
          0U,
          0U
      }),
      m_SwapChainImages({}),
      m_DepthImage(),
      m_FrameBuffers({}),
      m_Objects({}),
      m_ObjectIDCounter(0U)
{
}

VulkanBufferManager::~VulkanBufferManager()
{
    try
    {
        Shutdown();
    }
    catch (...)
    {
    }
}

VulkanBufferManager& VulkanBufferManager::Get()
{
    static VulkanBufferManager Instance{};
    return Instance;
}

void VulkanBufferManager::CreateMemoryAllocator()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan memory allocator";

    VmaVulkanFunctions const VulkanFunctions{
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr
    };

    VmaAllocatorCreateInfo const AllocatorInfo{
        .flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT,
        .physicalDevice = VulkanDeviceManager::Get().GetPhysicalDevice(),
        .device = VulkanDeviceManager::Get().GetLogicalDevice(),
        .preferredLargeHeapBlockSize = 0U /*Default: 256 MiB*/,
        .pAllocationCallbacks = nullptr,
        .pDeviceMemoryCallbacks = nullptr,
        .pHeapSizeLimit = nullptr,
        .pVulkanFunctions = &VulkanFunctions,
        .instance = VulkanRenderCore::Get().GetInstance(),
        .vulkanApiVersion = VK_API_VERSION_1_0,
        .pTypeExternalMemoryHandleTypes = nullptr
    };

    RenderCoreHelpers::CheckVulkanResult(vmaCreateAllocator(&AllocatorInfo, &m_Allocator));
}

void VulkanBufferManager::CreateSwapChain()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan swap chain";

    VulkanDeviceProperties const& Properties = VulkanDeviceManager::Get().GetDeviceProperties();

    std::vector<std::uint32_t> const QueueFamilyIndices = VulkanDeviceManager::Get().GetUniqueQueueFamilyIndicesU32();
    auto const QueueFamilyIndicesCount                  = static_cast<std::uint32_t>(QueueFamilyIndices.size());

    m_OldSwapChain    = m_SwapChain;
    m_SwapChainExtent = Properties.Extent;

    VkSwapchainCreateInfoKHR const SwapChainCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = VulkanRenderCore::Get().GetSurface(),
        .minImageCount = VulkanDeviceManager::Get().GetMinImageCount(),
        .imageFormat = Properties.Format.format,
        .imageColorSpace = Properties.Format.colorSpace,
        .imageExtent = m_SwapChainExtent,
        .imageArrayLayers = 1U,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = QueueFamilyIndicesCount > 1U ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = QueueFamilyIndicesCount,
        .pQueueFamilyIndices = QueueFamilyIndices.data(),
        .preTransform = Properties.Capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = Properties.Mode,
        .clipped = VK_TRUE,
        .oldSwapchain = m_OldSwapChain
    };

    VkDevice const& VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

    RenderCoreHelpers::CheckVulkanResult(vkCreateSwapchainKHR(VulkanLogicalDevice, &SwapChainCreateInfo, nullptr, &m_SwapChain));

    if (m_OldSwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(VulkanLogicalDevice, m_OldSwapChain, nullptr);
        m_OldSwapChain = VK_NULL_HANDLE;
    }

    std::uint32_t Count = 0U;
    RenderCoreHelpers::CheckVulkanResult(vkGetSwapchainImagesKHR(VulkanLogicalDevice, m_SwapChain, &Count, nullptr));

    std::vector<VkImage> SwapChainImages(Count, VK_NULL_HANDLE);
    RenderCoreHelpers::CheckVulkanResult(vkGetSwapchainImagesKHR(VulkanLogicalDevice, m_SwapChain, &Count, SwapChainImages.data()));

    m_SwapChainImages.resize(Count, VulkanImageAllocation());
    for (std::uint32_t Iterator = 0U; Iterator < Count; ++Iterator)
    {
        m_SwapChainImages[Iterator].Image = SwapChainImages[Iterator];
    }

    CreateSwapChainImageViews(Properties.Format.format);
}

void VulkanBufferManager::CreateFrameBuffers()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan frame buffers";

    VkDevice const& VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

    m_FrameBuffers.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);
    for (std::uint32_t Iterator = 0U; Iterator < static_cast<std::uint32_t>(m_FrameBuffers.size()); ++Iterator)
    {
        std::array const Attachments{
            m_SwapChainImages[Iterator].View,
            m_DepthImage.View
        };

        VkFramebufferCreateInfo const FrameBufferCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = VulkanPipelineManager::Get().GetRenderPass(),
            .attachmentCount = static_cast<std::uint32_t>(Attachments.size()),
            .pAttachments = Attachments.data(),
            .width = m_SwapChainExtent.width,
            .height = m_SwapChainExtent.height,
            .layers = 1U
        };

        RenderCoreHelpers::CheckVulkanResult(vkCreateFramebuffer(VulkanLogicalDevice, &FrameBufferCreateInfo, nullptr, &m_FrameBuffers[Iterator]));
    }
}

void VulkanBufferManager::CreateDepthResources()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan depth resources";

    VulkanDeviceProperties const& Properties = VulkanDeviceManager::Get().GetDeviceProperties();
    auto const& [FamilyIndex, Queue]         = VulkanDeviceManager::Get().GetGraphicsQueue();

    constexpr VkImageTiling Tiling                      = VK_IMAGE_TILING_OPTIMAL;
    constexpr VkImageUsageFlagBits Usage                = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    constexpr VkMemoryPropertyFlags MemoryPropertyFlags = 0U;
    constexpr VkImageAspectFlagBits Aspect              = VK_IMAGE_ASPECT_DEPTH_BIT;
    constexpr VkImageLayout InitialLayout               = VK_IMAGE_LAYOUT_UNDEFINED;
    constexpr VkImageLayout DestinationLayout           = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    CreateImage(Properties.DepthFormat, m_SwapChainExtent, Tiling, Usage, MemoryPropertyFlags, m_DepthImage.Image, m_DepthImage.Allocation);
    CreateImageView(m_DepthImage.Image, Properties.DepthFormat, Aspect, m_DepthImage.View);
    MoveImageLayout(m_DepthImage.Image, Properties.DepthFormat, InitialLayout, DestinationLayout, Queue, FamilyIndex);
}

void VulkanBufferManager::CreateVertexBuffers(VulkanObjectAllocation& Object, std::vector<Vertex> const& Vertices) const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan vertex buffers";

    if (m_Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    auto const& [FamilyIndex, Queue] = VulkanDeviceManager::Get().GetTransferQueue();

    constexpr VkBufferUsageFlags SourceUsageFlags             = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    constexpr VkBufferUsageFlags DestinationUsageFlags             = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = 0U;

    VkDeviceSize const BufferSize = Vertices.size() * sizeof(Vertex);

    VkBuffer StagingBuffer            = nullptr;
    VmaAllocation StagingBufferMemory = nullptr;
    VmaAllocationInfo const StagingInfo = CreateBuffer(BufferSize, SourceUsageFlags, SourceMemoryPropertyFlags, StagingBuffer, StagingBufferMemory); // NOLINT(readability-suspicious-call-argument)

    std::memcpy(StagingInfo.pMappedData, Vertices.data(), BufferSize);

    CreateBuffer(BufferSize, DestinationUsageFlags, DestinationMemoryPropertyFlags, Object.VertexBuffer.Buffer, Object.VertexBuffer.Allocation);
    CopyBuffer(StagingBuffer, Object.VertexBuffer.Buffer, BufferSize, Queue, FamilyIndex);

    vmaDestroyBuffer(m_Allocator, StagingBuffer, StagingBufferMemory);
}

void VulkanBufferManager::CreateIndexBuffers(VulkanObjectAllocation& Object, std::vector<std::uint32_t> const& Indices) const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan index buffers";

    if (m_Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    auto const& [FamilyIndex, Queue] = VulkanDeviceManager::Get().GetTransferQueue();

    constexpr VkBufferUsageFlags SourceUsageFlags             = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    constexpr VkBufferUsageFlags DestinationUsageFlags             = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = 0U;

    VkDeviceSize const BufferSize = Indices.size() * sizeof(std::uint32_t);

    VkBuffer StagingBuffer            = nullptr;
    VmaAllocation StagingBufferMemory = nullptr;
    VmaAllocationInfo const StagingInfo = CreateBuffer(BufferSize, SourceUsageFlags, SourceMemoryPropertyFlags, StagingBuffer, StagingBufferMemory); // NOLINT(readability-suspicious-call-argument)

    std::memcpy(StagingInfo.pMappedData, Indices.data(), BufferSize);
    RenderCoreHelpers::CheckVulkanResult(vmaFlushAllocation(m_Allocator, StagingBufferMemory, 0U, BufferSize));

    CreateBuffer(BufferSize, DestinationUsageFlags, DestinationMemoryPropertyFlags, Object.IndexBuffer.Buffer, Object.IndexBuffer.Allocation);
    CopyBuffer(StagingBuffer, Object.IndexBuffer.Buffer, BufferSize, Queue, FamilyIndex);

    vmaDestroyBuffer(m_Allocator, StagingBuffer, StagingBufferMemory);
}

std::uint64_t VulkanBufferManager::LoadObject(std::string_view const ModelPath, std::string_view const TexturePath)
{
    Assimp::Importer Importer;
    aiScene const* const Scene = Importer.ReadFile(ModelPath.data(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

    if (Scene == nullptr || (Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0U || Scene->mRootNode == nullptr)
    {
        throw std::runtime_error("Assimp error: " + std::string(Importer.GetErrorString()));
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loaded model from path: '" << ModelPath << "'";

    std::uint64_t const NewID = m_ObjectIDCounter.fetch_add(1U);

    std::vector<Vertex> Vertices;
    std::vector<std::uint32_t> Indices;

    std::span const AiMeshesSpan(Scene->mMeshes, Scene->mNumMeshes);
    for (std::vector const Meshes(AiMeshesSpan.begin(), AiMeshesSpan.end());
         aiMesh const* MeshIter : Meshes)
    {
        std::span const AiVerticesSpan(MeshIter->mVertices, MeshIter->mNumVertices);
        for (std::vector const VerticesSpan(AiVerticesSpan.begin(), AiVerticesSpan.end());
             aiVector3D const& Position : VerticesSpan)
        {
            auto const Index              = static_cast<std::uint32_t>(Vertices.size());
            aiVector3D const TextureCoord = MeshIter->mTextureCoords[0][Index]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

            Vertices.emplace_back(glm::vec3(Position.x, Position.y, Position.z), glm::vec3(1.F, 1.F, 1.F), glm::vec2(TextureCoord.x, TextureCoord.y));
        }

        std::span const AiFacesSpan(MeshIter->mFaces, MeshIter->mNumFaces);
        for (std::vector const Faces(AiFacesSpan.begin(), AiFacesSpan.end());
             aiFace const& Face : Faces)
        {
            std::span const AiIndicesSpan(Face.mIndices, Face.mNumIndices);
            for (std::vector const AiIndices(AiIndicesSpan.begin(), AiIndicesSpan.end());
                 std::uint32_t const IndiceIter : AiIndices)
            {
                Indices.emplace_back(IndiceIter);
            }
        }
    }

    VulkanObjectAllocation NewObject{
        .IndicesCount = static_cast<std::uint32_t>(Indices.size())
    };

    CreateVertexBuffers(NewObject, Vertices);
    CreateIndexBuffers(NewObject, Indices);
    LoadTexture(NewObject, TexturePath);

    m_Objects.emplace(NewID, NewObject);

    return NewID;
}

void VulkanBufferManager::UnLoadObject(std::uint64_t const ObjectID)
{
    if (!m_Objects.contains(ObjectID))
    {
        return;
    }

    m_Objects.at(ObjectID).DestroyResources();
    m_Objects.erase(ObjectID);
}

VulkanImageAllocation VulkanBufferManager::AllocateTexture(unsigned char const* Data, std::uint32_t const Width, std::uint32_t const Height, std::size_t const AllocationSize) const
{
    VulkanImageAllocation ImageAllocation{};

    constexpr VkBufferUsageFlags SourceUsageFlags             = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    constexpr VkImageUsageFlags DestinationUsageFlags              = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    constexpr VkImageTiling Tiling = VK_IMAGE_TILING_OPTIMAL;

    VkBuffer StagingBuffer            = nullptr;
    VmaAllocation StagingBufferMemory = nullptr;
    VmaAllocationInfo const StagingInfo = CreateBuffer(AllocationSize, SourceUsageFlags, SourceMemoryPropertyFlags, StagingBuffer, StagingBufferMemory); // NOLINT(readability-suspicious-call-argument)

    std::memcpy(StagingInfo.pMappedData, Data, AllocationSize);

    constexpr VkFormat ImageFormat            = VK_FORMAT_R8G8B8A8_SRGB;
    constexpr VkImageLayout InitialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    constexpr VkImageLayout MiddleLayout      = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    constexpr VkImageLayout DestinationLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkExtent2D const Extent{
        .width = Width,
        .height = Height
    };

    auto const& [FamilyIndex, Queue] = VulkanDeviceManager::Get().GetGraphicsQueue();

    CreateImage(ImageFormat, Extent, Tiling, DestinationUsageFlags, DestinationMemoryPropertyFlags, ImageAllocation.Image, ImageAllocation.Allocation);
    MoveImageLayout(ImageAllocation.Image, ImageFormat, InitialLayout, MiddleLayout, Queue, FamilyIndex);
    CopyBufferToImage(StagingBuffer, ImageAllocation.Image, Extent, Queue, FamilyIndex);
    MoveImageLayout(ImageAllocation.Image, ImageFormat, MiddleLayout, DestinationLayout, Queue, FamilyIndex);

    CreateTextureImageView(ImageAllocation);
    CreateTextureSampler(ImageAllocation);
    vmaDestroyBuffer(m_Allocator, StagingBuffer, StagingBufferMemory);

    return ImageAllocation;
}

void VulkanBufferManager::LoadTexture(VulkanObjectAllocation& Object, std::string_view const TexturePath) const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan texture image";

    if (m_Allocator == VK_NULL_HANDLE)
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

    stbi_uc const* const ImagePixels = stbi_load(UsedTexturePath.c_str(), &Width, &Height, &Channels, STBI_rgb_alpha);
    auto const AllocationSize        = static_cast<VkDeviceSize>(Width * Height * 4); // NOLINT(bugprone-misplaced-widening-cast)

    if (ImagePixels == nullptr)
    {
        throw std::runtime_error("STB Image is invalid. Path: " + std::string(TexturePath));
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loaded image from path: '" << TexturePath << "'";

    Object.TextureImage = AllocateTexture(ImagePixels, static_cast<std::uint32_t>(Width), static_cast<std::uint32_t>(Height), AllocationSize);
}

VmaAllocationInfo VulkanBufferManager::CreateBuffer(VkDeviceSize const& Size, VkBufferUsageFlags const Usage, VkMemoryPropertyFlags const Flags, VkBuffer& Buffer, VmaAllocation& Allocation) const
{
    if (m_Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    VmaAllocationCreateInfo const AllocationCreateInfo{
        .flags = Flags,
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    VkBufferCreateInfo const BufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = Size,
        .usage = Usage
    };

    VmaAllocationInfo MemoryAllocationInfo;
    RenderCoreHelpers::CheckVulkanResult(vmaCreateBuffer(m_Allocator, &BufferCreateInfo, &AllocationCreateInfo, &Buffer, &Allocation, &MemoryAllocationInfo));

    return MemoryAllocationInfo;
}

void VulkanBufferManager::CopyBuffer(VkBuffer const& Source, VkBuffer const& Destination, VkDeviceSize const& Size, VkQueue const& Queue, std::uint8_t const QueueFamilyIndex)
{
    VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
    VkCommandPool CommandPool     = VK_NULL_HANDLE;
    RenderCoreHelpers::InitializeSingleCommandQueue(CommandPool, CommandBuffer, QueueFamilyIndex);
    {
        VkBufferCopy const BufferCopy{
            .size = Size,
        };

        vkCmdCopyBuffer(CommandBuffer, Source, Destination, 1U, &BufferCopy);
    }
    RenderCoreHelpers::FinishSingleCommandQueue(Queue, CommandPool, CommandBuffer);
}

void VulkanBufferManager::CreateImage(VkFormat const& ImageFormat,
                                      VkExtent2D const& Extent,
                                      VkImageTiling const& Tiling,
                                      VkImageUsageFlags const Usage,
                                      VkMemoryPropertyFlags const Flags,
                                      VkImage& Image,
                                      VmaAllocation& Allocation) const
{
    if (m_Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    VkImageCreateInfo const ImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = ImageFormat,
        .extent = {
            .width = Extent.width,
            .height = Extent.height,
            .depth = 1U
        },
        .mipLevels = 1U,
        .arrayLayers = 1U,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = Tiling,
        .usage = Usage,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VmaAllocationCreateInfo const ImageAllocationCreateInfo{
        .flags = Flags,
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    VmaAllocationInfo AllocationInfo;
    RenderCoreHelpers::CheckVulkanResult(vmaCreateImage(m_Allocator, &ImageViewCreateInfo, &ImageAllocationCreateInfo, &Image, &Allocation, &AllocationInfo));
}

void VulkanBufferManager::CreateImageView(VkImage const& Image, VkFormat const& Format, VkImageAspectFlags const& AspectFlags, VkImageView& ImageView)
{
    VkImageViewCreateInfo const ImageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = Image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = Format,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange = {
            .aspectMask = AspectFlags,
            .baseMipLevel = 0U,
            .levelCount = 1U,
            .baseArrayLayer = 0U,
            .layerCount = 1U
        }
    };

    RenderCoreHelpers::CheckVulkanResult(vkCreateImageView(VulkanDeviceManager::Get().GetLogicalDevice(), &ImageViewCreateInfo, nullptr, &ImageView));
}

void VulkanBufferManager::CreateTextureImageView(VulkanImageAllocation& Allocation) const
{
    CreateImageView(Allocation.Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, Allocation.View);
}

void VulkanBufferManager::CreateTextureSampler(VulkanImageAllocation& Allocation) const
{
    if (m_Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    VkPhysicalDeviceProperties DeviceProperties;
    vkGetPhysicalDeviceProperties(m_Allocator->GetPhysicalDevice(), &DeviceProperties);

    VkSamplerCreateInfo const SamplerCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.F,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = DeviceProperties.limits.maxSamplerAnisotropy,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.F,
        .maxLod = FLT_MAX,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE
    };

    RenderCoreHelpers::CheckVulkanResult(vkCreateSampler(VulkanDeviceManager::Get().GetLogicalDevice(), &SamplerCreateInfo, nullptr, &Allocation.Sampler));
}

void VulkanBufferManager::CopyBufferToImage(VkBuffer const& Source, VkImage const& Destination, VkExtent2D const& Extent, VkQueue const& Queue, std::uint32_t const QueueFamilyIndex)
{
    VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
    VkCommandPool CommandPool     = VK_NULL_HANDLE;
    RenderCoreHelpers::InitializeSingleCommandQueue(CommandPool, CommandBuffer, static_cast<std::uint8_t>(QueueFamilyIndex));
    {
        VkBufferImageCopy const BufferImageCopy{
            .bufferOffset = 0U,
            .bufferRowLength = 0U,
            .bufferImageHeight = 0U,
            .imageSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0U,
                .baseArrayLayer = 0U,
                .layerCount = 1U
            },
            .imageOffset = {
                .x = 0U,
                .y = 0U,
                .z = 0U
            },
            .imageExtent = {
                .width = Extent.width,
                .height = Extent.height,
                .depth = 1U
            }
        };

        vkCmdCopyBufferToImage(CommandBuffer, Source, Destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &BufferImageCopy);
    }
    RenderCoreHelpers::FinishSingleCommandQueue(Queue, CommandPool, CommandBuffer);
}

void VulkanBufferManager::MoveImageLayout(VkImage const& Image,
                                          VkFormat const& Format,
                                          VkImageLayout const& OldLayout,
                                          VkImageLayout const& NewLayout,
                                          VkQueue const& Queue,
                                          std::uint8_t const QueueFamilyIndex)
{
    VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
    VkCommandPool CommandPool     = VK_NULL_HANDLE;
    RenderCoreHelpers::InitializeSingleCommandQueue(CommandPool, CommandBuffer, QueueFamilyIndex);
    {
        VkPipelineStageFlags SourceStage      = 0;
        VkPipelineStageFlags DestinationStage = 0;

        VkImageMemoryBarrier Barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .oldLayout = OldLayout,
            .newLayout = NewLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = Image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0U,
                .levelCount = 1U,
                .baseArrayLayer = 0U,
                .layerCount = 1U
            }
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

        vkCmdPipelineBarrier(CommandBuffer, SourceStage, DestinationStage, 0U, 0U, nullptr, 0U, nullptr, 1U, &Barrier);
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

    VkDevice const& VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

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

void VulkanBufferManager::DestroyResources(bool const ClearScene)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying Vulkan buffer manager resources";

    VkDevice const& VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

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

bool VulkanBufferManager::IsInitialized() const
{
    return m_Allocator != VK_NULL_HANDLE;
}

VmaAllocator const& VulkanBufferManager::GetAllocator() const
{
    return m_Allocator;
}

VkSwapchainKHR const& VulkanBufferManager::GetSwapChain() const
{
    return m_SwapChain;
}

VkExtent2D const& VulkanBufferManager::GetSwapChainExtent() const
{
    return m_SwapChainExtent;
}

std::vector<VkImage> VulkanBufferManager::GetSwapChainImages() const
{
    std::vector<VkImage> SwapChainImages;
    for (auto const& [Image, View, Sampler, Allocation] : m_SwapChainImages) // NOLINT(misc-misplaced-const)
    {
        SwapChainImages.push_back(Image);
    }

    return SwapChainImages;
}

std::vector<VkFramebuffer> const& VulkanBufferManager::GetFrameBuffers() const
{
    return m_FrameBuffers;
}

VkBuffer VulkanBufferManager::GetVertexBuffer(std::uint64_t const ObjectID) const
{
    if (!m_Objects.contains(ObjectID))
    {
        return VK_NULL_HANDLE;
    }

    return m_Objects.at(ObjectID).VertexBuffer.Buffer;
}

VkBuffer VulkanBufferManager::GetIndexBuffer(std::uint64_t const ObjectID) const
{
    if (!m_Objects.contains(ObjectID))
    {
        return VK_NULL_HANDLE;
    }

    return m_Objects.at(ObjectID).IndexBuffer.Buffer;
}

std::uint32_t VulkanBufferManager::GetIndicesCount(std::uint64_t const ObjectID) const
{
    if (!m_Objects.contains(ObjectID))
    {
        return 0U;
    }

    return m_Objects.at(ObjectID).IndicesCount;
}

std::vector<VulkanTextureData> VulkanBufferManager::GetAllocatedTextures() const
{
    std::vector<VulkanTextureData> Output;
    for (auto const& [TextureImage, VertexBuffer, IndexBuffer, IndicesCount] : m_Objects | std::views::values)
    {
        Output.emplace_back(TextureImage.View, TextureImage.Sampler);
    }
    return Output;
}

void VulkanBufferManager::CreateSwapChainImageViews(VkFormat const& ImageFormat)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan swap chain image views";
    for (auto& [Image, View, Sampler, Allocation] : m_SwapChainImages)
    {
        CreateImageView(Image, ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, View);
    }
}
