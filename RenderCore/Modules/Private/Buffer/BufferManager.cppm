// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <algorithm>
#include <boost/log/trivial.hpp>
#include <filesystem>
#include <glm/ext.hpp>
#include <ranges>
#include <span>
#include <volk.h>

#ifndef VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#endif
#include <vk_mem_alloc.h>

#ifndef TINYGLTF_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#endif
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#include <tiny_gltf.h>

#ifdef GLFW_INCLUDE_VULKAN
#undef GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

module RenderCore.Management.BufferManagement;

import RenderCore.Renderer;
import RenderCore.Management.CommandsManagement;
import RenderCore.Management.DeviceManagement;
import RenderCore.Management.PipelineManagement;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Types.Vertex;
import RenderCore.Types.Camera;
import RenderCore.Types.Object;
import Timer.ExecutionCounter;

using namespace RenderCore;

VmaAllocationInfo CreateBuffer(VmaAllocator const& Allocator, VkDeviceSize const& Size, VkBufferUsageFlags const Usage, VkMemoryPropertyFlags const Flags, VkBuffer& Buffer, VmaAllocation& Allocation)
{
    if (Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    VmaAllocationCreateInfo const AllocationCreateInfo {
            .flags = Flags,
            .usage = VMA_MEMORY_USAGE_AUTO};

    VkBufferCreateInfo const BufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size  = std::clamp(Size, g_BufferMemoryAllocationSize, UINT64_MAX),
            .usage = Usage};

    VmaAllocationInfo MemoryAllocationInfo;
    CheckVulkanResult(vmaCreateBuffer(Allocator, &BufferCreateInfo, &AllocationCreateInfo, &Buffer, &Allocation, &MemoryAllocationInfo));

    return MemoryAllocationInfo;
}

void CopyBuffer(VkCommandBuffer const& CommandBuffer, VkBuffer const& Source, VkBuffer const& Destination, VkDeviceSize const& Size)
{
    VkBufferCopy const BufferCopy {
            .size = Size,
    };

    vkCmdCopyBuffer(CommandBuffer, Source, Destination, 1U, &BufferCopy);
}

CommandBufferSet CreateVertexBuffers(VmaAllocator const& Allocator, ObjectAllocation& Object, std::vector<Vertex> const& Vertices)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan vertex buffers";

    if (Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    VkDeviceSize const BufferSize = std::size(Vertices) * sizeof(Vertex);

    constexpr VkBufferUsageFlags SourceUsageFlags             = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    constexpr VkBufferUsageFlags DestinationUsageFlags             = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = 0U;

    CommandBufferSet Output {
            .bVertexBuffer  = true,
            .AllocationSize = BufferSize};

    VmaAllocationInfo const
            StagingInfo
            = CreateBuffer(Allocator,
                           BufferSize,
                           SourceUsageFlags,
                           SourceMemoryPropertyFlags,
                           Output.StagingBuffer.first,
                           Output.StagingBuffer.second);

    std::memcpy(StagingInfo.pMappedData, std::data(Vertices), BufferSize);

    CreateBuffer(Allocator, BufferSize, DestinationUsageFlags, DestinationMemoryPropertyFlags, Object.VertexBuffer.Buffer, Object.VertexBuffer.Allocation);

    return Output;
}

CommandBufferSet CreateIndexBuffers(VmaAllocator const& Allocator, ObjectAllocation& Object, std::vector<std::uint32_t> const& Indices)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan index buffers";

    if (Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    VkDeviceSize const BufferSize = std::size(Indices) * sizeof(std::uint32_t);

    constexpr VkBufferUsageFlags SourceUsageFlags             = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    constexpr VkBufferUsageFlags DestinationUsageFlags             = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = 0U;

    CommandBufferSet Output {
            .bVertexBuffer  = false,
            .AllocationSize = BufferSize};

    VmaAllocationInfo const
            StagingInfo
            = CreateBuffer(Allocator,
                           BufferSize,
                           SourceUsageFlags,
                           SourceMemoryPropertyFlags,
                           Output.StagingBuffer.first,
                           Output.StagingBuffer.second);

    std::memcpy(StagingInfo.pMappedData, std::data(Indices), BufferSize);
    CheckVulkanResult(vmaFlushAllocation(Allocator, Output.StagingBuffer.second, 0U, BufferSize));

    CreateBuffer(Allocator, BufferSize, DestinationUsageFlags, DestinationMemoryPropertyFlags, Object.IndexBuffer.Buffer, Object.IndexBuffer.Allocation);

    return Output;
}

void CreateUniformBuffers(VmaAllocator const& Allocator, ObjectAllocation& Object)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan uniform buffers";

    if (Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    constexpr VkDeviceSize BufferSize = sizeof(UniformBufferObject);

    constexpr VkBufferUsageFlags DestinationUsageFlags             = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    CreateBuffer(Allocator, BufferSize, DestinationUsageFlags, DestinationMemoryPropertyFlags, Object.UniformBuffer.Buffer, Object.UniformBuffer.Allocation);
    vmaMapMemory(Allocator, Object.UniformBuffer.Allocation, &Object.UniformBuffer.MappedData);
}

void CreateImage(VmaAllocator const& Allocator,
                 VkFormat const& ImageFormat,
                 VkExtent2D const& Extent,
                 VkImageTiling const& Tiling,
                 VkImageUsageFlags const Usage,
                 VkMemoryPropertyFlags const Flags,
                 VkImage& Image,
                 VmaAllocation& Allocation)
{
    if (Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    VkImageCreateInfo const ImageViewCreateInfo {
            .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format    = ImageFormat,
            .extent    = {
                       .width  = Extent.width,
                       .height = Extent.height,
                       .depth  = 1U},
            .mipLevels     = 1U,
            .arrayLayers   = 1U,
            .samples       = VK_SAMPLE_COUNT_1_BIT,
            .tiling        = Tiling,
            .usage         = Usage,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};

    VmaAllocationCreateInfo const ImageCreateInfo {
            .flags = Flags,
            .usage = VMA_MEMORY_USAGE_AUTO};

    VmaAllocationInfo AllocationInfo;
    CheckVulkanResult(vmaCreateImage(Allocator, &ImageViewCreateInfo, &ImageCreateInfo, &Image, &Allocation, &AllocationInfo));
}

void CreateImageView(VkImage const& Image, VkFormat const& Format, VkImageAspectFlags const& AspectFlags, VkImageView& ImageView)
{
    VkImageViewCreateInfo const ImageViewCreateInfo {
            .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image      = Image,
            .viewType   = VK_IMAGE_VIEW_TYPE_2D,
            .format     = Format,
            .components = {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY},
            .subresourceRange = {.aspectMask = AspectFlags, .baseMipLevel = 0U, .levelCount = 1U, .baseArrayLayer = 0U, .layerCount = 1U}};

    CheckVulkanResult(vkCreateImageView(volkGetLoadedDevice(), &ImageViewCreateInfo, nullptr, &ImageView));
}

void CreateSwapChainImageViews(std::vector<ImageAllocation>& Images, VkFormat const& ImageFormat)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan swap chain image views";
    for (auto& [Image, View, Sampler, Allocation, Type]: Images)
    {
        CreateImageView(Image, ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, View);
    }
}

void CreateTextureImageView(ImageAllocation& Allocation)
{
    CreateImageView(Allocation.Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, Allocation.View);
}

void CreateTextureSampler(VmaAllocator const& Allocator, ImageAllocation& Allocation)
{
    if (Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    VkPhysicalDeviceProperties SurfaceProperties;
    vkGetPhysicalDeviceProperties(Allocator->GetPhysicalDevice(), &SurfaceProperties);

    VkSamplerCreateInfo const SamplerCreateInfo {
            .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter               = VK_FILTER_LINEAR,
            .minFilter               = VK_FILTER_LINEAR,
            .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias              = 0.F,
            .anisotropyEnable        = VK_TRUE,
            .maxAnisotropy           = SurfaceProperties.limits.maxSamplerAnisotropy,
            .compareEnable           = VK_FALSE,
            .compareOp               = VK_COMPARE_OP_ALWAYS,
            .minLod                  = 0.F,
            .maxLod                  = FLT_MAX,
            .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE};

    CheckVulkanResult(vkCreateSampler(volkGetLoadedDevice(), &SamplerCreateInfo, nullptr, &Allocation.Sampler));
}

void CopyBufferToImage(VkCommandBuffer const& CommandBuffer, VkBuffer const& Source, VkImage const& Destination, VkExtent2D const& Extent)
{
    VkBufferImageCopy const BufferImageCopy {
            .bufferOffset      = 0U,
            .bufferRowLength   = 0U,
            .bufferImageHeight = 0U,
            .imageSubresource  = {
                     .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                     .mipLevel       = 0U,
                     .baseArrayLayer = 0U,
                     .layerCount     = 1U},
            .imageOffset = {.x = 0U, .y = 0U, .z = 0U},
            .imageExtent = {.width = Extent.width, .height = Extent.height, .depth = 1U}};

    vkCmdCopyBufferToImage(CommandBuffer, Source, Destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &BufferImageCopy);
}

template<VkImageLayout OldLayout, VkImageLayout NewLayout>
constexpr void MoveImageLayout(VkCommandBuffer& CommandBuffer, VkImage const& Image, VkFormat const& Format)
{
    VkPipelineStageFlags SourceStage;
    VkPipelineStageFlags DestinationStage;

    VkImageMemoryBarrier Barrier = {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .oldLayout           = OldLayout,
            .newLayout           = NewLayout,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = Image,
            .subresourceRange    = {
                       .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                       .baseMipLevel   = 0U,
                       .levelCount     = 1U,
                       .baseArrayLayer = 0U,
                       .layerCount     = 1U}};

    if constexpr (NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (Format == VK_FORMAT_D32_SFLOAT_S8_UINT || Format == VK_FORMAT_D24_UNORM_S8_UINT)
        {
            Barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }

    if constexpr (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        Barrier.srcAccessMask = 0;
        Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        SourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        DestinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if constexpr (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        SourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
        DestinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if constexpr (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
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

ImageCreationData AllocateTexture(VmaAllocator const& Allocator, unsigned char const* Data, std::uint32_t const Width, std::uint32_t const Height, std::size_t const AllocationSize)
{
    ImageAllocation ImageAllocation {};

    constexpr VkBufferUsageFlags SourceUsageFlags             = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    constexpr VkImageUsageFlags DestinationUsageFlags              = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    constexpr VkImageTiling Tiling = VK_IMAGE_TILING_OPTIMAL;

    ImageCreationData Output {};

    VmaAllocationInfo const
            StagingInfo
            = CreateBuffer(Allocator,
                           std::clamp(AllocationSize, g_ImageBufferMemoryAllocationSize, UINT64_MAX),
                           SourceUsageFlags,
                           SourceMemoryPropertyFlags,
                           Output.StagingBuffer.first,
                           Output.StagingBuffer.second);

    std::memcpy(StagingInfo.pMappedData, Data, AllocationSize);

    constexpr VkFormat ImageFormat = VK_FORMAT_R8G8B8A8_SRGB;

    VkExtent2D const Extent {
            .width  = Width,
            .height = Height};

    CreateImage(Allocator, ImageFormat, Extent, Tiling, DestinationUsageFlags, DestinationMemoryPropertyFlags, ImageAllocation.Image, ImageAllocation.Allocation);

    Output.Allocation = ImageAllocation;
    Output.Format     = ImageFormat;
    Output.Extent     = Extent;

    return Output;
}

void BufferManager::CreateVulkanSurface(GLFWwindow* const Window)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan surface";

    CheckVulkanResult(glfwCreateWindowSurface(volkGetLoadedInstance(), Window, nullptr, &m_Surface));
}

void BufferManager::CreateMemoryAllocator(VkPhysicalDevice const& PhysicalDevice)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan memory allocator";

    VmaVulkanFunctions const VulkanFunctions {
            .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
            .vkGetDeviceProcAddr   = vkGetDeviceProcAddr};

    VmaAllocatorCreateInfo const AllocatorInfo {
            .flags                          = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT,
            .physicalDevice                 = PhysicalDevice,
            .device                         = volkGetLoadedDevice(),
            .preferredLargeHeapBlockSize    = 0U /*Default: 256 MiB*/,
            .pAllocationCallbacks           = nullptr,
            .pDeviceMemoryCallbacks         = nullptr,
            .pHeapSizeLimit                 = nullptr,
            .pVulkanFunctions               = &VulkanFunctions,
            .instance                       = volkGetLoadedInstance(),
            .vulkanApiVersion               = VK_API_VERSION_1_0,
            .pTypeExternalMemoryHandleTypes = nullptr};

    CheckVulkanResult(vmaCreateAllocator(&AllocatorInfo, &m_Allocator));
}

void BufferManager::CreateSwapChain(SurfaceProperties const& SurfaceProperties, VkSurfaceCapabilitiesKHR const& Capabilities, std::vector<std::uint32_t> const& QueueFamilyIndices)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan swap chain";

    auto const QueueFamilyIndicesCount = static_cast<std::uint32_t>(std::size(QueueFamilyIndices));

    m_OldSwapChain    = m_SwapChain;
    m_SwapChainExtent = SurfaceProperties.Extent;

    VkSwapchainCreateInfoKHR const SwapChainCreateInfo {
            .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface               = GetSurface(),
            .minImageCount         = g_MinImageCount,
            .imageFormat           = SurfaceProperties.Format.format,
            .imageColorSpace       = SurfaceProperties.Format.colorSpace,
            .imageExtent           = m_SwapChainExtent,
            .imageArrayLayers      = 1U,
            .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode      = QueueFamilyIndicesCount > 1U ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = QueueFamilyIndicesCount,
            .pQueueFamilyIndices   = std::data(QueueFamilyIndices),
            .preTransform          = Capabilities.currentTransform,
            .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode           = SurfaceProperties.Mode,
            .clipped               = VK_TRUE,
            .oldSwapchain          = m_OldSwapChain};

    CheckVulkanResult(vkCreateSwapchainKHR(volkGetLoadedDevice(), &SwapChainCreateInfo, nullptr, &m_SwapChain));

    if (m_OldSwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(volkGetLoadedDevice(), m_OldSwapChain, nullptr);
        m_OldSwapChain = VK_NULL_HANDLE;
    }

    std::uint32_t Count = 0U;
    CheckVulkanResult(vkGetSwapchainImagesKHR(volkGetLoadedDevice(), m_SwapChain, &Count, nullptr));

    std::vector<VkImage> SwapChainImages(Count, VK_NULL_HANDLE);
    CheckVulkanResult(vkGetSwapchainImagesKHR(volkGetLoadedDevice(), m_SwapChain, &Count, std::data(SwapChainImages)));

    m_SwapChainImages.resize(Count, ImageAllocation());
    for (std::uint32_t Iterator = 0U; Iterator < Count; ++Iterator)
    {
        m_SwapChainImages.at(Iterator).Image = SwapChainImages.at(Iterator);
    }

    CreateSwapChainImageViews(m_SwapChainImages, SurfaceProperties.Format.format);
}

void BufferManager::CreateFrameBuffers(VkRenderPass const& RenderPass)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan frame buffers";

    m_FrameBuffers.resize(std::size(m_SwapChainImages), VK_NULL_HANDLE);
    for (std::uint32_t Iterator = 0U; Iterator < static_cast<std::uint32_t>(std::size(m_FrameBuffers)); ++Iterator)
    {
        std::array const Attachments {
                m_SwapChainImages.at(Iterator).View,
                m_DepthImage.View};

        VkFramebufferCreateInfo const FrameBufferCreateInfo {
                .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass      = RenderPass,
                .attachmentCount = static_cast<std::uint32_t>(std::size(Attachments)),
                .pAttachments    = std::data(Attachments),
                .width           = m_SwapChainExtent.width,
                .height          = m_SwapChainExtent.height,
                .layers          = 1U};

        CheckVulkanResult(vkCreateFramebuffer(volkGetLoadedDevice(), &FrameBufferCreateInfo, nullptr, &m_FrameBuffers.at(Iterator)));
    }
}

void BufferManager::CreateDepthResources(SurfaceProperties const& SurfaceProperties, std::pair<std::uint8_t, VkQueue> const& QueuePair)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan depth resources";

    auto const& [FamilyIndex, Queue] = QueuePair;

    constexpr VkImageTiling Tiling                      = VK_IMAGE_TILING_OPTIMAL;
    constexpr VkImageUsageFlagBits Usage                = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    constexpr VkMemoryPropertyFlags MemoryPropertyFlags = 0U;
    constexpr VkImageAspectFlagBits Aspect              = VK_IMAGE_ASPECT_DEPTH_BIT;
    constexpr VkImageLayout InitialLayout               = VK_IMAGE_LAYOUT_UNDEFINED;
    constexpr VkImageLayout DestinationLayout           = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    CreateImage(m_Allocator, SurfaceProperties.DepthFormat, m_SwapChainExtent, Tiling, Usage, MemoryPropertyFlags, m_DepthImage.Image, m_DepthImage.Allocation);
    CreateImageView(m_DepthImage.Image, SurfaceProperties.DepthFormat, Aspect, m_DepthImage.View);

    VkCommandPool CommandPool {VK_NULL_HANDLE};
    std::vector<VkCommandBuffer> CommandBuffer {VK_NULL_HANDLE};
    InitializeSingleCommandQueue(CommandPool, CommandBuffer, FamilyIndex);
    {
        MoveImageLayout<InitialLayout, DestinationLayout>(CommandBuffer.back(), m_DepthImage.Image, SurfaceProperties.DepthFormat);
    }
    FinishSingleCommandQueue(Queue, CommandPool, CommandBuffer);
}

void TryResizeVertexContainer(std::vector<Vertex>& Vertices, std::uint32_t const NewSize)
{
    if (std::size(Vertices) < NewSize && NewSize > 0U)
    {
        Vertices.resize(NewSize);
    }
}

void InsertIndiceInContainer(std::vector<std::uint32_t>& Indices, tinygltf::Accessor const& IndexAccessor, auto const* Data)
{
    for (std::uint32_t Iterator = 0U; Iterator < static_cast<std::uint32_t>(IndexAccessor.count); ++Iterator)
    {
        Indices.push_back(static_cast<std::uint32_t>(Data[Iterator]));
    }
}

void AllocateModelTexture(ObjectAllocationData& ObjectCreationData, tinygltf::Model const& Model, VmaAllocator const& Allocator, std::int32_t const TextureIndex, TextureType const TextureType)
{
    if (TextureIndex >= 0)
    {
        if (tinygltf::Texture const& Texture = Model.textures.at(TextureIndex);
            Texture.source >= 0)
        {
            tinygltf::Image const& Image = Model.images.at(Texture.source);

            ObjectCreationData.ImageCreationDatas.push_back(AllocateTexture(Allocator, std::data(Image.image), Image.width, Image.height, std::size(Image.image)));
            ObjectCreationData.ImageCreationDatas.back().Type = TextureType;
        }
    }
}

std::vector<Object> BufferManager::AllocateScene(std::string_view const& ModelPath, std::pair<std::uint8_t, VkQueue> const& QueuePair)
{
    std::vector<ObjectAllocationData> AllocationData {};
    tinygltf::Model Model {};

    {
        tinygltf::TinyGLTF ModelLoader {};
        std::string Error {};
        std::string Warning {};
        std::filesystem::path const ModelFilepath(ModelPath);
        bool const LoadResult = ModelFilepath.extension() == ".gltf" ? ModelLoader.LoadASCIIFromFile(&Model, &Error, &Warning, std::data(ModelPath)) : ModelLoader.LoadBinaryFromFile(&Model, &Error, &Warning, std::data(ModelPath));
        if (!std::empty(Error))
        {
            BOOST_LOG_TRIVIAL(error) << "[" << __func__ << "]: Error: '" << Error << "'";
        }

        if (!std::empty(Warning))
        {
            BOOST_LOG_TRIVIAL(warning) << "[" << __func__ << "]: Warning: '" << Warning << "'";
        }

        if (!LoadResult)
        {
            BOOST_LOG_TRIVIAL(error) << "[" << __func__ << "]: Failed to load model from path: '" << ModelPath << "'";
            return {};
        }
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loaded model from path: '" << ModelPath << "'";
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Loading scenes: " << std::size(Model.scenes);

    for (tinygltf::Node const& Node: Model.nodes)
    {
        std::int32_t const MeshIndex = Node.mesh;
        if (MeshIndex < 0)
        {
            continue;
        }

        tinygltf::Mesh const& Mesh = Model.meshes.at(MeshIndex);
        AllocationData.reserve(std::size(AllocationData) + std::size(Mesh.primitives));

        for (tinygltf::Primitive const& Primitive: Mesh.primitives)
        {
            std::uint32_t const ObjectID = m_ObjectIDCounter.fetch_add(1U);

            ObjectAllocationData NewObjectAllocation {
                    .Object = {ObjectID, ModelPath, std::format("{}_{:03d}", Mesh.name, ObjectID)}};

            {
                const float* PositionData {nullptr};
                const float* NormalData {nullptr};
                const float* TexCoordData {nullptr};

                if (Primitive.attributes.contains("POSITION"))
                {
                    tinygltf::Accessor const& PositionAccessor     = Model.accessors.at(Primitive.attributes.at("POSITION"));
                    tinygltf::BufferView const& PositionBufferView = Model.bufferViews.at(PositionAccessor.bufferView);
                    tinygltf::Buffer const& PositionBuffer         = Model.buffers.at(PositionBufferView.buffer);
                    PositionData                                   = reinterpret_cast<const float*>(std::data(PositionBuffer.data) + PositionBufferView.byteOffset + PositionAccessor.byteOffset);
                    TryResizeVertexContainer(NewObjectAllocation.Vertices, PositionAccessor.count);
                }

                if (Primitive.attributes.contains("NORMAL"))
                {
                    tinygltf::Accessor const& NormalAccessor     = Model.accessors.at(Primitive.attributes.at("NORMAL"));
                    tinygltf::BufferView const& NormalBufferView = Model.bufferViews.at(NormalAccessor.bufferView);
                    tinygltf::Buffer const& NormalBuffer         = Model.buffers.at(NormalBufferView.buffer);
                    NormalData                                   = reinterpret_cast<const float*>(std::data(NormalBuffer.data) + NormalBufferView.byteOffset + NormalAccessor.byteOffset);
                    TryResizeVertexContainer(NewObjectAllocation.Vertices, NormalAccessor.count);
                }

                if (Primitive.attributes.contains("TEXCOORD_0"))
                {
                    tinygltf::Accessor const& TexCoordAccessor     = Model.accessors.at(Primitive.attributes.at("TEXCOORD_0"));
                    tinygltf::BufferView const& TexCoordBufferView = Model.bufferViews.at(TexCoordAccessor.bufferView);
                    tinygltf::Buffer const& TexCoordBuffer         = Model.buffers.at(TexCoordBufferView.buffer);
                    TexCoordData                                   = reinterpret_cast<const float*>(std::data(TexCoordBuffer.data) + TexCoordBufferView.byteOffset + TexCoordAccessor.byteOffset);
                    TryResizeVertexContainer(NewObjectAllocation.Vertices, TexCoordAccessor.count);
                }

                for (std::uint32_t Iterator = 0U; Iterator < static_cast<std::uint32_t>(std::size(NewObjectAllocation.Vertices)); ++Iterator)
                {
                    if (PositionData)
                    {
                        NewObjectAllocation.Vertices.at(Iterator).Position = glm::make_vec3(&PositionData[Iterator * 3]);
                    }

                    if (NormalData)
                    {
                        NewObjectAllocation.Vertices.at(Iterator).Normal = glm::make_vec3(&NormalData[Iterator * 3]);
                    }

                    if (TexCoordData)
                    {
                        NewObjectAllocation.Vertices.at(Iterator).TextureCoordinate = glm::make_vec2(&TexCoordData[Iterator * 2]);
                    }
                }
            }

            if (Primitive.indices >= 0)
            {
                tinygltf::Accessor const& IndexAccessor     = Model.accessors.at(Primitive.indices);
                tinygltf::BufferView const& IndexBufferView = Model.bufferViews.at(IndexAccessor.bufferView);
                tinygltf::Buffer const& IndexBuffer         = Model.buffers.at(IndexBufferView.buffer);
                unsigned char const* const IndicesData      = std::data(IndexBuffer.data) + IndexBufferView.byteOffset + IndexAccessor.byteOffset;

                NewObjectAllocation.Indices.reserve(IndexAccessor.count);
                NewObjectAllocation.Object.SetTrianglesCount(IndexAccessor.count / 3U);

                switch (IndexAccessor.componentType)
                {
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                        InsertIndiceInContainer(NewObjectAllocation.Indices, IndexAccessor, reinterpret_cast<const uint32_t*>(IndicesData));
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                        InsertIndiceInContainer(NewObjectAllocation.Indices, IndexAccessor, reinterpret_cast<const uint16_t*>(IndicesData));
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                        InsertIndiceInContainer(NewObjectAllocation.Indices, IndexAccessor, IndicesData);
                        break;
                    }
                    default:
                        break;
                }
            }

            if (!std::empty(Node.translation))
            {
                NewObjectAllocation.Object.SetPosition(Vector(glm::make_vec3(std::data(Node.translation))));
            }

            if (!std::empty(Node.scale))
            {
                NewObjectAllocation.Object.SetScale(Vector(glm::make_vec3(std::data(Node.scale))));
            }

            if (!std::empty(Node.rotation))
            {
                NewObjectAllocation.Object.SetRotation(Rotator(glm::make_quat(std::data(Node.rotation))));
            }

            if (Primitive.material >= 0)
            {
                tinygltf::Material const& Material = Model.materials.at(Primitive.material);
                AllocateModelTexture(NewObjectAllocation, Model, m_Allocator, Material.pbrMetallicRoughness.baseColorTexture.index, TextureType::BaseColor);

                if (std::empty(NewObjectAllocation.ImageCreationDatas))
                {
                    constexpr std::uint8_t DefaultTextureHalfSize {2U};
                    constexpr std::uint8_t DefaultTextureSize {DefaultTextureHalfSize * 2U};
                    constexpr std::array<std::uint8_t, DefaultTextureSize> DefaultTextureData {};

                    NewObjectAllocation.ImageCreationDatas.push_back(AllocateTexture(m_Allocator,
                                                                                     std::data(DefaultTextureData),
                                                                                     DefaultTextureHalfSize,
                                                                                     DefaultTextureHalfSize,
                                                                                     DefaultTextureSize));

                    NewObjectAllocation.ImageCreationDatas.back().Type = TextureType::BaseColor;
                }
            }

            NewObjectAllocation.CommandBufferSets.push_back(CreateVertexBuffers(m_Allocator, NewObjectAllocation.Allocation, NewObjectAllocation.Vertices));
            NewObjectAllocation.CommandBufferSets.push_back(CreateIndexBuffers(m_Allocator, NewObjectAllocation.Allocation, NewObjectAllocation.Indices));
            CreateUniformBuffers(m_Allocator, NewObjectAllocation.Allocation);

            AllocationData.push_back(NewObjectAllocation);
        }
    }

    std::vector<Object> Output;
    Output.reserve(std::size(AllocationData));

    m_Objects.reserve(std::size(m_Objects) + std::size(AllocationData));

    std::vector<BufferCopyOperationData> BufferCopyOperationDatas {};
    BufferCopyOperationDatas.reserve(std::size(AllocationData));

    std::vector<MoveOperationData> MoveOperation {};
    std::vector<CopyOperationData> CopyOperation {};

    for (ObjectAllocationData const& AllocationDataIter: AllocationData)
    {
        BufferCopyOperationDatas.emplace_back();
        BufferCopyOperationData& NewCopyOperationData = BufferCopyOperationDatas.back();

        NewCopyOperationData.VertexData.SourceBuffer          = AllocationDataIter.CommandBufferSets.at(0U).StagingBuffer.first;
        NewCopyOperationData.VertexData.SourceAllocation      = AllocationDataIter.CommandBufferSets.at(0U).StagingBuffer.second;
        NewCopyOperationData.VertexData.DestinationBuffer     = AllocationDataIter.Allocation.VertexBuffer.Buffer;
        NewCopyOperationData.VertexData.DestinationAllocation = AllocationDataIter.Allocation.VertexBuffer.Allocation;
        NewCopyOperationData.VertexData.AllocationSize        = AllocationDataIter.CommandBufferSets.at(0U).AllocationSize;

        NewCopyOperationData.IndexData.SourceBuffer          = AllocationDataIter.CommandBufferSets.at(1U).StagingBuffer.first;
        NewCopyOperationData.IndexData.SourceAllocation      = AllocationDataIter.CommandBufferSets.at(1U).StagingBuffer.second;
        NewCopyOperationData.IndexData.DestinationBuffer     = AllocationDataIter.Allocation.IndexBuffer.Buffer;
        NewCopyOperationData.IndexData.DestinationAllocation = AllocationDataIter.Allocation.IndexBuffer.Allocation;
        NewCopyOperationData.IndexData.AllocationSize        = AllocationDataIter.CommandBufferSets.at(1U).AllocationSize;

        Output.push_back(AllocationDataIter.Object);

        m_Objects.emplace(AllocationDataIter.Object.GetID(), ObjectAllocation {
                                                                     .VertexBuffer  = AllocationDataIter.Allocation.VertexBuffer,
                                                                     .IndexBuffer   = AllocationDataIter.Allocation.IndexBuffer,
                                                                     .UniformBuffer = AllocationDataIter.Allocation.UniformBuffer,
                                                                     .IndicesCount  = static_cast<std::uint32_t>(std::size(AllocationDataIter.Indices))});

        MoveOperation.reserve(std::size(MoveOperation) + std::size(AllocationDataIter.ImageCreationDatas));
        for (ImageCreationData const& ImageDataIter: AllocationDataIter.ImageCreationDatas)
        {
            MoveOperation.push_back({.Image  = ImageDataIter.Allocation.Image,
                                     .Format = ImageDataIter.Format});

            CopyOperation.push_back({.SourceBuffer     = ImageDataIter.StagingBuffer.first,
                                     .SourceAllocation = ImageDataIter.StagingBuffer.second,
                                     .DestinationImage = ImageDataIter.Allocation.Image,
                                     .Format           = ImageDataIter.Format,
                                     .Extent           = ImageDataIter.Extent});
        }
    }

    VkCommandPool CopyCommandPool {VK_NULL_HANDLE};
    std::vector<VkCommandBuffer> CommandBuffers {};
    CommandBuffers.resize(std::size(AllocationData) + std::size(MoveOperation) + std::size(CopyOperation) + std::size(MoveOperation));

    InitializeSingleCommandQueue(CopyCommandPool, CommandBuffers, QueuePair.first);
    {
        std::uint32_t Count = 0U;

        for (BufferCopyOperationData const& CopyOperationDataIter: BufferCopyOperationDatas)
        {
            VkCommandBuffer const& CommandBuffer = CommandBuffers.at(Count);

            VkBufferCopy const VertexBufferCopy {
                    .size = CopyOperationDataIter.VertexData.AllocationSize};

            VkBufferCopy const IndexBufferCopy {
                    .size = CopyOperationDataIter.IndexData.AllocationSize};

            vkCmdCopyBuffer(CommandBuffer, CopyOperationDataIter.VertexData.SourceBuffer, CopyOperationDataIter.VertexData.DestinationBuffer, 1U, &VertexBufferCopy);
            vkCmdCopyBuffer(CommandBuffer, CopyOperationDataIter.IndexData.SourceBuffer, CopyOperationDataIter.IndexData.DestinationBuffer, 1U, &IndexBufferCopy);

            ++Count;
        }

        for (MoveOperationData const& MoveOperationIter: MoveOperation)
        {
            MoveImageLayout<VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL>(CommandBuffers.at(Count),
                                                                                             MoveOperationIter.Image,
                                                                                             MoveOperationIter.Format);

            ++Count;
        }

        for (CopyOperationData const& CopyOperationIter: CopyOperation)
        {
            CopyBufferToImage(CommandBuffers.at(Count),
                              CopyOperationIter.SourceBuffer,
                              CopyOperationIter.DestinationImage,
                              CopyOperationIter.Extent);

            ++Count;
        }

        for (MoveOperationData const& MoveOperationIter: MoveOperation)
        {
            MoveImageLayout<VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL>(CommandBuffers.at(Count),
                                                                      MoveOperationIter.Image,
                                                                      MoveOperationIter.Format);

            ++Count;
        }
    }
    FinishSingleCommandQueue(QueuePair.second, CopyCommandPool, CommandBuffers);

    for (ObjectAllocationData& AllocationDataIter: AllocationData)
    {
        for (ImageCreationData& ImageDataIter: AllocationDataIter.ImageCreationDatas)
        {
            if (m_Objects.contains(AllocationDataIter.Object.GetID()))
            {
                CreateTextureImageView(ImageDataIter.Allocation);
                CreateTextureSampler(m_Allocator, ImageDataIter.Allocation);
                m_Objects.at(AllocationDataIter.Object.GetID()).TextureImages.push_back(ImageDataIter.Allocation);
            }

            vmaDestroyBuffer(m_Allocator, ImageDataIter.StagingBuffer.first, ImageDataIter.StagingBuffer.second);
        }

        for (CommandBufferSet& CommandBufferSetIter: AllocationDataIter.CommandBufferSets)
        {
            vmaDestroyBuffer(m_Allocator, CommandBufferSetIter.StagingBuffer.first, CommandBufferSetIter.StagingBuffer.second);
        }
    }

    return Output;
}

void BufferManager::ReleaseScene(std::vector<std::uint32_t> const& ObjectIDs)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    for (std::uint32_t const ObjectIDIter: ObjectIDs)
    {
        if (!m_Objects.contains(ObjectIDIter))
        {
            return;
        }

        m_Objects.at(ObjectIDIter).DestroyResources(m_Allocator);
        m_Objects.erase(ObjectIDIter);
    }

    if (std::empty(m_Objects))
    {
        m_ObjectIDCounter = 0U;
    }
}

void BufferManager::ReleaseBufferResources()
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Releasing vulkan buffer resources";

    if (m_SwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(volkGetLoadedDevice(), m_SwapChain, nullptr);
        m_SwapChain = VK_NULL_HANDLE;
    }

    DestroyBufferResources(true);

    vkDestroySurfaceKHR(volkGetLoadedInstance(), m_Surface, nullptr);
    m_Surface = VK_NULL_HANDLE;

    vmaDestroyAllocator(m_Allocator);
    m_Allocator = VK_NULL_HANDLE;
}

void BufferManager::DestroyBufferResources(bool const ClearScene)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying vulkan buffer resources";

    for (ImageAllocation& ImageViewIter: m_SwapChainImages)
    {
        ImageViewIter.DestroyResources(m_Allocator);
    }
    m_SwapChainImages.clear();

    for (VkFramebuffer& FrameBufferIter: m_FrameBuffers)
    {
        if (FrameBufferIter != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(volkGetLoadedDevice(), FrameBufferIter, nullptr);
            FrameBufferIter = VK_NULL_HANDLE;
        }
    }
    m_FrameBuffers.clear();

    m_DepthImage.DestroyResources(m_Allocator);

    if (ClearScene)
    {
        for (auto& ObjectIter: m_Objects | std::views::values)
        {
            ObjectIter.DestroyResources(m_Allocator);
        }
        m_Objects.clear();
    }
}

VkSurfaceKHR const& BufferManager::GetSurface() const
{
    return m_Surface;
}

VkSwapchainKHR const& BufferManager::GetSwapChain() const
{
    return m_SwapChain;
}

VkExtent2D const& BufferManager::GetSwapChainExtent() const
{
    return m_SwapChainExtent;
}

std::vector<VkFramebuffer> const& BufferManager::GetFrameBuffers() const
{
    return m_FrameBuffers;
}

VkBuffer BufferManager::GetVertexBuffer(std::uint32_t const ObjectID) const
{
    if (!m_Objects.contains(ObjectID))
    {
        return VK_NULL_HANDLE;
    }

    return m_Objects.at(ObjectID).VertexBuffer.Buffer;
}

VkBuffer BufferManager::GetIndexBuffer(std::uint32_t const ObjectID) const
{
    if (!m_Objects.contains(ObjectID))
    {
        return VK_NULL_HANDLE;
    }

    return m_Objects.at(ObjectID).IndexBuffer.Buffer;
}

std::uint32_t BufferManager::GetIndicesCount(std::uint32_t const ObjectID) const
{
    if (!m_Objects.contains(ObjectID))
    {
        return 0U;
    }

    return m_Objects.at(ObjectID).IndicesCount;
}

void* BufferManager::GetUniformData(std::uint32_t const ObjectID) const
{
    if (!m_Objects.contains(ObjectID))
    {
        return nullptr;
    }

    return m_Objects.at(ObjectID).UniformBuffer.MappedData;
}

bool BufferManager::ContainsObject(std::uint32_t const ID) const
{
    return m_Objects.contains(ID);
}

std::vector<MeshBufferData> BufferManager::GetAllocatedObjects() const
{
    std::vector<MeshBufferData> Output;
    for (auto const& [ID, Data]: m_Objects)
    {
        if (!Data.UniformBuffer.Allocation)
        {
            continue;
        }

        Output.push_back({.ID                = ID,
                          .UniformBuffer     = Data.UniformBuffer.Buffer,
                          .UniformBufferData = Data.UniformBuffer.Allocation->GetMappedData()});

        for (auto const& ImageAllocation: Data.TextureImages)
        {
            Output.back().Textures.emplace(ImageAllocation.Type,
                                           TextureBufferData {
                                                   .ImageView = ImageAllocation.View,
                                                   .Sampler   = ImageAllocation.Sampler});
        }
    }

    return Output;
}

std::uint32_t BufferManager::GetNumAllocations() const
{
    return static_cast<std::uint32_t>(std::size(m_Objects));
}

std::uint32_t BufferManager::GetClampedNumAllocations() const
{
    return std::clamp(static_cast<std::uint32_t>(std::size(m_Objects)), 1U, UINT32_MAX);
}

void BufferManager::UpdateUniformBuffers(std::shared_ptr<Object> const& Object, Camera const& Camera, VkExtent2D const& Extent) const
{
    if (!Object)
    {
        return;
    }

    if (void* UniformBufferData = GetUniformData(Object->GetID()))
    {
        UniformBufferObject const UpdatedUBO {
                .Model      = Object->GetMatrix(),
                .View       = Camera.GetViewMatrix(),
                .Projection = Camera.GetProjectionMatrix(Extent)};

        std::memcpy(UniformBufferData, &UpdatedUBO, sizeof(UniformBufferObject));
    }
}

VmaAllocator const& BufferManager::GetAllocator() const
{
    return m_Allocator;
}