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
import RenderCore.Management.AllocationRegister;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Types.Vertex;
import RenderCore.Types.Camera;
import RenderCore.Types.Object;
import RuntimeInfo.Manager;

using namespace RenderCore;

VmaAllocationInfo CreateBuffer(VmaAllocator const& Allocator,
                               VkDeviceSize const& Size,
                               VkBufferUsageFlags const Usage,
                               VkMemoryPropertyFlags const Flags,
                               std::string_view const Identifier,
                               VkBuffer& Buffer,
                               VmaAllocation& Allocation)
{
    RuntimeInfo::Manager::Get().PushCallstack();

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
    CheckVulkanResult(vmaCreateBuffer(Allocator,
                                      &BufferCreateInfo,
                                      &AllocationCreateInfo,
                                      &Buffer,
                                      &Allocation,
                                      &MemoryAllocationInfo));

    vmaSetAllocationName(Allocator, Allocation, std::format("Buffer: {}", Identifier).c_str());

    return MemoryAllocationInfo;
}

void CopyBuffer(VkCommandBuffer const& CommandBuffer,
                VkBuffer const& Source,
                VkBuffer const& Destination,
                VkDeviceSize const& Size)
{
    RuntimeInfo::Manager::Get().PushCallstack();

    VkBufferCopy const BufferCopy {
            .size = Size,
    };

    vkCmdCopyBuffer(CommandBuffer, Source, Destination, 1U, &BufferCopy);
}

CommandBufferSet CreateVertexBuffers(VmaAllocator const& Allocator,
                                     ObjectAllocation& Object,
                                     std::vector<Vertex> const& Vertices)
{
    RuntimeInfo::Manager::Get().PushCallstack();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating Vulkan vertex buffers";

    if (Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    VkDeviceSize const BufferSize = std::size(Vertices) * sizeof(Vertex);

    constexpr VkBufferUsageFlags SourceUsageFlags                  = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags      = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
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
                           "STAGING_VERTEX",
                           Output.StagingBuffer.first,
                           Output.StagingBuffer.second);

    std::memcpy(StagingInfo.pMappedData, std::data(Vertices), BufferSize);

    CreateBuffer(Allocator,
                 BufferSize,
                 DestinationUsageFlags,
                 DestinationMemoryPropertyFlags,
                 "VERTEX",
                 Object.VertexBuffer.Buffer,
                 Object.VertexBuffer.Allocation);

    return Output;
}

CommandBufferSet CreateIndexBuffers(VmaAllocator const& Allocator,
                                    ObjectAllocation& Object,
                                    std::vector<std::uint32_t> const& Indices)
{
    RuntimeInfo::Manager::Get().PushCallstack();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating Vulkan index buffers";

    if (Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    VkDeviceSize const BufferSize = std::size(Indices) * sizeof(std::uint32_t);

    constexpr VkBufferUsageFlags SourceUsageFlags                  = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VkMemoryPropertyFlags SourceMemoryPropertyFlags      = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
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
                           "STAGING_INDEX",
                           Output.StagingBuffer.first,
                           Output.StagingBuffer.second);

    std::memcpy(StagingInfo.pMappedData, std::data(Indices), BufferSize);
    CheckVulkanResult(vmaFlushAllocation(Allocator, Output.StagingBuffer.second, 0U, BufferSize));

    CreateBuffer(Allocator,
                 BufferSize,
                 DestinationUsageFlags,
                 DestinationMemoryPropertyFlags,
                 "INDEX",
                 Object.IndexBuffer.Buffer,
                 Object.IndexBuffer.Allocation);

    return Output;
}

void CreateUniformBuffers(VmaAllocator const& Allocator, ObjectAllocation& Object)
{
    RuntimeInfo::Manager::Get().PushCallstack();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating Vulkan uniform buffers";

    if (Allocator == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan memory allocator is invalid.");
    }

    constexpr VkDeviceSize BufferSize = sizeof(UniformBufferObject);

    constexpr VkBufferUsageFlags DestinationUsageFlags             = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    constexpr VkMemoryPropertyFlags DestinationMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    CreateBuffer(Allocator,
                 BufferSize,
                 DestinationUsageFlags,
                 DestinationMemoryPropertyFlags,
                 "UNIFORM",
                 Object.UniformBuffer.Buffer,
                 Object.UniformBuffer.Allocation);
    vmaMapMemory(Allocator, Object.UniformBuffer.Allocation, &Object.UniformBuffer.MappedData);
}

void CreateImage(VmaAllocator const& Allocator,
                 VkFormat const& ImageFormat,
                 VkExtent2D const& Extent,
                 VkImageTiling const& Tiling,
                 VkImageUsageFlags const ImageUsage,
                 VmaAllocationCreateFlags const Flags,
                 VmaMemoryUsage const MemoryUsage,
                 std::string_view const Identifier,
                 VkImage& Image,
                 VmaAllocation& Allocation)
{
    RuntimeInfo::Manager::Get().PushCallstack();

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
            .samples       = g_MSAASamples,
            .tiling        = Tiling,
            .usage         = ImageUsage,
            .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};

    VmaAllocationCreateInfo const ImageCreateInfo {
            .flags = Flags,
            .usage = MemoryUsage};

    VmaAllocationInfo AllocationInfo;
    CheckVulkanResult(vmaCreateImage(Allocator,
                                     &ImageViewCreateInfo,
                                     &ImageCreateInfo,
                                     &Image,
                                     &Allocation,
                                     &AllocationInfo));

    vmaSetAllocationName(Allocator, Allocation, std::format("Image: {}", Identifier).c_str());
}

void CreateTextureSampler(VmaAllocator const& Allocator, VkSampler& Sampler)
{
    RuntimeInfo::Manager::Get().PushCallstack();

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

    CheckVulkanResult(vkCreateSampler(volkGetLoadedDevice(), &SamplerCreateInfo, nullptr, &Sampler));
}

void CreateImageView(VkImage const& Image,
                     VkFormat const& Format,
                     VkImageAspectFlags const& AspectFlags,
                     VkImageView& ImageView)
{
    RuntimeInfo::Manager::Get().PushCallstack();

    VkImageViewCreateInfo const ImageViewCreateInfo {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image            = Image,
            .viewType         = VK_IMAGE_VIEW_TYPE_2D,
            .format           = Format,
            .subresourceRange = {
                    .aspectMask     = AspectFlags,
                    .baseMipLevel   = 0U,
                    .levelCount     = 1U,
                    .baseArrayLayer = 0U,
                    .layerCount     = 1U}};

    CheckVulkanResult(vkCreateImageView(volkGetLoadedDevice(), &ImageViewCreateInfo, nullptr, &ImageView));
}

void CreateSwapChainImageViews(std::vector<ImageAllocation>& Images, VkFormat const& ImageFormat)
{
    auto const _ {RuntimeInfo::Manager::Get().PushCallstackWithCounter()};
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating vulkan swap chain image views";

    for (auto& [Image, View, Allocation, Type]: Images)
    {
        CreateImageView(Image, ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, View);
    }
}

void CreateTextureImageView(ImageAllocation& Allocation)
{
    RuntimeInfo::Manager::Get().PushCallstack();
    CreateImageView(Allocation.Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, Allocation.View);
}

void CopyBufferToImage(VkCommandBuffer const& CommandBuffer,
                       VkBuffer const& Source,
                       VkImage const& Destination,
                       VkExtent2D const& Extent)
{
    RuntimeInfo::Manager::Get().PushCallstack();

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

    vkCmdCopyBufferToImage(CommandBuffer,
                           Source,
                           Destination,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1U,
                           &BufferImageCopy);
}

ImageCreationData AllocateTexture(VmaAllocator const& Allocator,
                                  unsigned char const* Data,
                                  std::uint32_t const Width,
                                  std::uint32_t const Height,
                                  std::size_t const AllocationSize)
{
    RuntimeInfo::Manager::Get().PushCallstack();

    ImageAllocation ImageAllocation {};

    constexpr VmaMemoryUsage MemoryUsage = VMA_MEMORY_USAGE_AUTO;

    constexpr VkBufferUsageFlags SourceUsageFlags                = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    constexpr VmaAllocationCreateFlags SourceMemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    constexpr VkImageUsageFlags DestinationUsageFlags                 = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    constexpr VmaAllocationCreateFlags DestinationMemoryPropertyFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    constexpr VkImageTiling Tiling = VK_IMAGE_TILING_OPTIMAL;

    ImageCreationData Output {};

    VmaAllocationInfo const
            StagingInfo
            = CreateBuffer(Allocator,
                           std::clamp(AllocationSize, g_ImageBufferMemoryAllocationSize, UINT64_MAX),
                           SourceUsageFlags,
                           SourceMemoryPropertyFlags,
                           "STAGING_TEXTURE",
                           Output.StagingBuffer.first,
                           Output.StagingBuffer.second);

    std::memcpy(StagingInfo.pMappedData, Data, AllocationSize);

    constexpr VkFormat ImageFormat = VK_FORMAT_R8G8B8A8_SRGB;

    VkExtent2D const Extent {
            .width  = Width,
            .height = Height};

    CreateImage(Allocator,
                ImageFormat,
                Extent,
                Tiling,
                DestinationUsageFlags,
                DestinationMemoryPropertyFlags,
                MemoryUsage,
                "TEXTURE",
                ImageAllocation.Image,
                ImageAllocation.Allocation);

    Output.Allocation = ImageAllocation;
    Output.Format     = ImageFormat;
    Output.Extent     = Extent;

    return Output;
}

void BufferManager::CreateVulkanSurface(GLFWwindow* const Window)
{
    auto const _ {RuntimeInfo::Manager::Get().PushCallstackWithCounter()};
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating vulkan surface";

    CheckVulkanResult(glfwCreateWindowSurface(volkGetLoadedInstance(), Window, nullptr, &m_Surface));
}

void BufferManager::CreateMemoryAllocator(VkPhysicalDevice const& PhysicalDevice)
{
    auto const _ {RuntimeInfo::Manager::Get().PushCallstackWithCounter()};
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating vulkan memory allocator";

    VmaVulkanFunctions const VulkanFunctions {
            .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
            .vkGetDeviceProcAddr   = vkGetDeviceProcAddr};

    constexpr VmaDeviceMemoryCallbacks AllocationCallbacks {
            .pfnAllocate = AllocationRegister::AllocateDeviceMemoryCallback,
            .pfnFree     = AllocationRegister::FreeDeviceMemoryCallback};

    VmaAllocatorCreateInfo const AllocatorInfo {
            .flags                          = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT | VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT,
            .physicalDevice                 = PhysicalDevice,
            .device                         = volkGetLoadedDevice(),
            .preferredLargeHeapBlockSize    = 0U /*Default: 256 MiB*/,
            .pAllocationCallbacks           = nullptr,
            .pDeviceMemoryCallbacks         = &AllocationCallbacks,
            .pHeapSizeLimit                 = nullptr,
            .pVulkanFunctions               = &VulkanFunctions,
            .instance                       = volkGetLoadedInstance(),
            .vulkanApiVersion               = VK_API_VERSION_1_3,
            .pTypeExternalMemoryHandleTypes = nullptr};

    CheckVulkanResult(vmaCreateAllocator(&AllocatorInfo, &m_Allocator));
}

void BufferManager::CreateImageSampler()
{
    auto const _ {RuntimeInfo::Manager::Get().PushCallstackWithCounter()};
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating vulkan image sampler";

    CreateTextureSampler(m_Allocator, m_Sampler);
}

void BufferManager::CreateSwapChain(SurfaceProperties const& SurfaceProperties,
                                    VkSurfaceCapabilitiesKHR const& Capabilities)
{
    auto const _ {RuntimeInfo::Manager::Get().PushCallstackWithCounter()};
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating Vulkan swap chain";

    auto const QueueFamilyIndices      = GetUniqueQueueFamilyIndicesU32();
    auto const QueueFamilyIndicesCount = static_cast<std::uint32_t>(std::size(QueueFamilyIndices));

    m_OldSwapChain         = m_SwapChain;
    m_SwapChainExtent      = SurfaceProperties.Extent;
    m_SwapChainImageFormat = SurfaceProperties.Format.format;

    VkSwapchainCreateInfoKHR const SwapChainCreateInfo {
            .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface               = GetSurface(),
            .minImageCount         = g_MinImageCount,
            .imageFormat           = m_SwapChainImageFormat,
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

void BufferManager::CreateViewportResources(SurfaceProperties const& SurfaceProperties)
{
    auto const _ {RuntimeInfo::Manager::Get().PushCallstackWithCounter()};
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating Vulkan viewport resources";

    for (ImageAllocation& ImageIter: m_ViewportImages)
    {
        ImageIter.DestroyResources(m_Allocator);
    }
    m_ViewportImages.resize(std::size(m_SwapChainImages));

    constexpr VmaMemoryUsage MemoryUsage                   = VMA_MEMORY_USAGE_AUTO;
    constexpr VkImageAspectFlags AspectFlags               = VK_IMAGE_ASPECT_COLOR_BIT;
    constexpr VkImageTiling Tiling                         = VK_IMAGE_TILING_LINEAR;
    constexpr VkImageUsageFlags UsageFlags                 = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    constexpr VmaAllocationCreateFlags MemoryPropertyFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

    for (auto& [Image, View, Allocation, Type]: m_ViewportImages)
    {
        CreateImage(m_Allocator,
                    SurfaceProperties.Format.format,
                    SurfaceProperties.Extent,
                    Tiling,
                    UsageFlags,
                    MemoryPropertyFlags,
                    MemoryUsage,
                    "VIEWPORT",
                    Image,
                    Allocation);

        CreateImageView(Image, SurfaceProperties.Format.format, AspectFlags, View);
    }
}

void BufferManager::CreateDepthResources(SurfaceProperties const& SurfaceProperties)
{
    auto const _ {RuntimeInfo::Manager::Get().PushCallstackWithCounter()};
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating vulkan depth resources";

    if (m_DepthImage.IsValid())
    {
        m_DepthImage.DestroyResources(m_Allocator);
    }

    constexpr VmaMemoryUsage MemoryUsage                   = VMA_MEMORY_USAGE_AUTO;
    constexpr VkImageTiling Tiling                         = VK_IMAGE_TILING_OPTIMAL;
    constexpr VkImageUsageFlagBits Usage                   = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    constexpr VmaAllocationCreateFlags MemoryPropertyFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    constexpr VkImageAspectFlagBits Aspect                 = VK_IMAGE_ASPECT_DEPTH_BIT;

    m_DepthFormat = SurfaceProperties.DepthFormat;

    CreateImage(m_Allocator,
                m_DepthFormat,
                SurfaceProperties.Extent,
                Tiling,
                Usage,
                MemoryPropertyFlags,
                MemoryUsage,
                "DEPTH",
                m_DepthImage.Image,
                m_DepthImage.Allocation);

    CreateImageView(m_DepthImage.Image, m_DepthFormat, Aspect, m_DepthImage.View);
}

void TryResizeVertexContainer(std::vector<Vertex>& Vertices, std::uint32_t const NewSize)
{
    RuntimeInfo::Manager::Get().PushCallstack();

    if (std::size(Vertices) < NewSize && NewSize > 0U)
    {
        Vertices.resize(NewSize);
    }
}

void InsertIndiceInContainer(std::vector<std::uint32_t>& Indices,
                             tinygltf::Accessor const& IndexAccessor,
                             auto const* Data)
{
    RuntimeInfo::Manager::Get().PushCallstack();

    for (std::uint32_t Iterator = 0U; Iterator < static_cast<std::uint32_t>(IndexAccessor.count); ++Iterator)
    {
        Indices.push_back(static_cast<std::uint32_t>(Data[Iterator]));
    }
}

void AllocateModelTexture(ObjectAllocationData& ObjectCreationData,
                          tinygltf::Model const& Model,
                          VmaAllocator const& Allocator,
                          std::int32_t const TextureIndex,
                          TextureType const TextureType)
{
    RuntimeInfo::Manager::Get().PushCallstack();

    if (TextureIndex >= 0)
    {
        if (tinygltf::Texture const& Texture = Model.textures.at(TextureIndex);
            Texture.source >= 0)
        {
            tinygltf::Image const& Image = Model.images.at(Texture.source);

            ObjectCreationData.ImageCreationDatas.push_back(
                    AllocateTexture(Allocator, std::data(Image.image), Image.width, Image.height, std::size(Image.image)));
            ObjectCreationData.ImageCreationDatas.back().Type = TextureType;
        }
    }
}

std::vector<Object> BufferManager::AllocateScene(std::string_view const ModelPath)
{
    RuntimeInfo::Manager::Get().PushCallstack();

    std::vector<ObjectAllocationData> AllocationData {};
    tinygltf::Model Model {};
    {
        tinygltf::TinyGLTF ModelLoader {};
        std::string Error {};
        std::string Warning {};
        std::filesystem::path const ModelFilepath(ModelPath);
        bool const LoadResult = ModelFilepath.extension() == ".gltf"
                                        ? ModelLoader.LoadASCIIFromFile(&Model, &Error, &Warning, std::data(ModelPath))
                                        : ModelLoader.LoadBinaryFromFile(&Model, &Error, &Warning, std::data(ModelPath));
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

    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Loaded model from path: '" << ModelPath << "'";
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Loading scenes: " << std::size(Model.scenes);

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
                    PositionData                                   = reinterpret_cast<const float*>(
                            std::data(PositionBuffer.data) + PositionBufferView.byteOffset + PositionAccessor.byteOffset);
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
                    TexCoordData                                   = reinterpret_cast<const float*>(
                            std::data(TexCoordBuffer.data) + TexCoordBufferView.byteOffset + TexCoordAccessor.byteOffset);
                    TryResizeVertexContainer(NewObjectAllocation.Vertices, TexCoordAccessor.count);
                }

                for (std::uint32_t Iterator = 0U; Iterator < static_cast<std::uint32_t>(std::size(
                                                          NewObjectAllocation.Vertices));
                     ++Iterator)
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
                        NewObjectAllocation.Vertices.at(Iterator).TextureCoordinate = glm::make_vec2(
                                &TexCoordData[Iterator * 2]);
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
                        InsertIndiceInContainer(NewObjectAllocation.Indices,
                                                IndexAccessor,
                                                reinterpret_cast<const uint32_t*>(IndicesData));
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                        InsertIndiceInContainer(NewObjectAllocation.Indices,
                                                IndexAccessor,
                                                reinterpret_cast<const uint16_t*>(IndicesData));
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
                AllocateModelTexture(NewObjectAllocation,
                                     Model,
                                     m_Allocator,
                                     Material.pbrMetallicRoughness.baseColorTexture.index,
                                     TextureType::BaseColor);

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

            NewObjectAllocation.CommandBufferSets.push_back(
                    CreateVertexBuffers(m_Allocator, NewObjectAllocation.Allocation, NewObjectAllocation.Vertices));

            NewObjectAllocation.CommandBufferSets.push_back(
                    CreateIndexBuffers(m_Allocator, NewObjectAllocation.Allocation, NewObjectAllocation.Indices));

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

    for (auto const& [Object, Allocation, Vertices, Indices, ImageCreationDatas, CommandBufferSets]: AllocationData)
    {
        BufferCopyOperationDatas.emplace_back();
        auto& [VertexData, IndexData] = BufferCopyOperationDatas.back();

        VertexData.SourceBuffer          = CommandBufferSets.at(0U).StagingBuffer.first;
        VertexData.SourceAllocation      = CommandBufferSets.at(0U).StagingBuffer.second;
        VertexData.DestinationBuffer     = Allocation.VertexBuffer.Buffer;
        VertexData.DestinationAllocation = Allocation.VertexBuffer.Allocation;
        VertexData.AllocationSize        = CommandBufferSets.at(0U).AllocationSize;

        IndexData.SourceBuffer          = CommandBufferSets.at(1U).StagingBuffer.first;
        IndexData.SourceAllocation      = CommandBufferSets.at(1U).StagingBuffer.second;
        IndexData.DestinationBuffer     = Allocation.IndexBuffer.Buffer;
        IndexData.DestinationAllocation = Allocation.IndexBuffer.Allocation;
        IndexData.AllocationSize        = CommandBufferSets.at(1U).AllocationSize;

        Output.push_back(Object);

        m_Objects.emplace(Object.GetID(),
                          ObjectAllocation {
                                  .VertexBuffer  = Allocation.VertexBuffer,
                                  .IndexBuffer   = Allocation.IndexBuffer,
                                  .UniformBuffer = Allocation.UniformBuffer,
                                  .IndicesCount  = static_cast<std::uint32_t>(std::size(Indices))});

        MoveOperation.reserve(std::size(MoveOperation) + std::size(ImageCreationDatas));
        for (auto const& [Allocation, StagingBuffer, Format, Extent, Type]: ImageCreationDatas)
        {
            MoveOperation.push_back({.Image  = Allocation.Image,
                                     .Format = Format});

            CopyOperation.push_back({.SourceBuffer     = StagingBuffer.first,
                                     .SourceAllocation = StagingBuffer.second,
                                     .DestinationImage = Allocation.Image,
                                     .Format           = Format,
                                     .Extent           = Extent});
        }
    }

    auto const& [FamilyIndex, Queue] = GetGraphicsQueue();

    VkCommandPool CopyCommandPool {VK_NULL_HANDLE};
    std::vector<VkCommandBuffer> CommandBuffers {};
    CommandBuffers.resize(
            std::size(AllocationData) + std::size(MoveOperation) + std::size(CopyOperation) + std::size(MoveOperation));

    InitializeSingleCommandQueue(CopyCommandPool, CommandBuffers, FamilyIndex);
    {
        std::uint32_t Count = 0U;

        for (auto const& [VertexData, IndexData]: BufferCopyOperationDatas)
        {
            VkCommandBuffer const& CommandBuffer = CommandBuffers.at(Count);

            VkBufferCopy const VertexBufferCopy {
                    .size = VertexData.AllocationSize};

            VkBufferCopy const IndexBufferCopy {
                    .size = IndexData.AllocationSize};

            vkCmdCopyBuffer(CommandBuffer,
                            VertexData.SourceBuffer,
                            VertexData.DestinationBuffer,
                            1U,
                            &VertexBufferCopy);

            vkCmdCopyBuffer(CommandBuffer,
                            IndexData.SourceBuffer,
                            IndexData.DestinationBuffer,
                            1U,
                            &IndexBufferCopy);

            ++Count;
        }

        for (auto const& [Image, Format]: MoveOperation)
        {
            MoveImageLayout<VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_IMAGE_ASPECT_COLOR_BIT>(CommandBuffers.at(Count), Image, Format);

            ++Count;
        }

        for (auto const& [SourceBuffer, SourceAllocation, DestinationImage, Format, Extent]: CopyOperation)
        {
            CopyBufferToImage(CommandBuffers.at(Count),
                              SourceBuffer,
                              DestinationImage,
                              Extent);

            ++Count;
        }

        for (auto const& [Image, Format]: MoveOperation)
        {
            MoveImageLayout<VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR,
                            VK_IMAGE_ASPECT_COLOR_BIT>(CommandBuffers.at(Count), Image, Format);

            ++Count;
        }
    }
    FinishSingleCommandQueue(Queue, CopyCommandPool, CommandBuffers);

    for (auto& [Object, Allocation, Vertices, Indices, ImageCreationDatas, CommandBufferSets]: AllocationData)
    {
        for (auto& [Allocation, StagingBuffer, Format, Extent, Type]: ImageCreationDatas)
        {
            if (m_Objects.contains(Object.GetID()))
            {
                CreateTextureImageView(Allocation);
                m_Objects.at(Object.GetID()).TextureImages.push_back(Allocation);
            }

            vmaDestroyBuffer(m_Allocator, StagingBuffer.first, StagingBuffer.second);
        }

        for (auto& [bVertexBuffer, CommandBuffer, AllocationSize, StagingBuffer]: CommandBufferSets)
        {
            vmaDestroyBuffer(m_Allocator,
                             StagingBuffer.first,
                             StagingBuffer.second);
        }
    }

    return Output;
}

void BufferManager::ReleaseScene(std::vector<std::uint32_t> const& ObjectIDs)
{
    auto const _ {RuntimeInfo::Manager::Get().PushCallstackWithCounter()};

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
    auto const _ {RuntimeInfo::Manager::Get().PushCallstackWithCounter()};
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Releasing vulkan buffer resources";

    if (m_SwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(volkGetLoadedDevice(), m_SwapChain, nullptr);
        m_SwapChain = VK_NULL_HANDLE;
    }

    if (m_OldSwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(volkGetLoadedDevice(), m_OldSwapChain, nullptr);
        m_OldSwapChain = VK_NULL_HANDLE;
    }

    if (m_Sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(volkGetLoadedDevice(), m_Sampler, nullptr);
        m_Sampler = VK_NULL_HANDLE;
    }

    DestroyBufferResources(true);

    vkDestroySurfaceKHR(volkGetLoadedInstance(), m_Surface, nullptr);
    m_Surface = VK_NULL_HANDLE;

    vmaDestroyAllocator(m_Allocator);
    m_Allocator = VK_NULL_HANDLE;
}

void BufferManager::DestroyBufferResources(bool const ClearScene)
{
    auto const _ {RuntimeInfo::Manager::Get().PushCallstackWithCounter()};
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Destroying vulkan buffer resources";

    for (ImageAllocation& ImageViewIter: m_SwapChainImages)
    {
        ImageViewIter.DestroyResources(m_Allocator);
    }
    m_SwapChainImages.clear();

    for (ImageAllocation& ImageViewIter: m_ViewportImages)
    {
        ImageViewIter.DestroyResources(m_Allocator);
    }
    m_ViewportImages.clear();

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

VkFormat const& BufferManager::GetSwapChainImageFormat() const
{
    return m_SwapChainImageFormat;
}

std::vector<ImageAllocation> const& BufferManager::GetSwapChainImages() const
{
    return m_SwapChainImages;
}

std::vector<ImageAllocation> const& BufferManager::GetViewportImages() const
{
    return m_ViewportImages;
}

ImageAllocation const& BufferManager::GetDepthImage() const
{
    return m_DepthImage;
}

VkFormat const& BufferManager::GetDepthFormat() const
{
    return m_DepthFormat;
}

VkSampler const& BufferManager::GetSampler() const
{
    return m_Sampler;
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

        for (const auto& [Image, View, Allocation, Type]: Data.TextureImages)
        {
            Output.back().Textures.emplace(Type, View);
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

void BufferManager::UpdateUniformBuffers(std::shared_ptr<Object> const& Object,
                                         Camera const& Camera,
                                         VkExtent2D const& Extent) const
{
    RuntimeInfo::Manager::Get().PushCallstack();

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

void BufferManager::SaveImageToFile(VkImage const& Image, std::string_view const Path) const
{
    RuntimeInfo::Manager::Get().PushCallstack();

    VkBuffer Buffer;
    VmaAllocation Allocation;
    VkSubresourceLayout Layout;

    std::uint32_t const ImageWidth {m_SwapChainExtent.width};
    std::uint32_t const ImageHeight {m_SwapChainExtent.height};
    constexpr std::uint8_t Components {4U};

    auto const& [FamilyIndex, Queue] = GetGraphicsQueue();

    VkCommandPool CommandPool {VK_NULL_HANDLE};
    std::vector<VkCommandBuffer> CommandBuffer {VK_NULL_HANDLE};
    InitializeSingleCommandQueue(CommandPool, CommandBuffer, FamilyIndex);
    {
        VkImageSubresource SubResource {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel   = 0,
                .arrayLayer = 0};

        vkGetImageSubresourceLayout(volkGetLoadedDevice(), Image, &SubResource, &Layout);

        VkBufferCreateInfo BufferInfo {
                .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size        = Layout.size,
                .usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE};

        VmaAllocationCreateInfo AllocationInfo {
                .usage = VMA_MEMORY_USAGE_CPU_ONLY};

        vmaCreateBuffer(m_Allocator, &BufferInfo, &AllocationInfo, &Buffer, &Allocation, nullptr);

        VkBufferImageCopy Region {
                .bufferOffset      = 0U,
                .bufferRowLength   = 0U,
                .bufferImageHeight = 0U,
                .imageSubresource  = {
                         .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                         .mipLevel       = 0U,
                         .baseArrayLayer = 0U,
                         .layerCount     = 1U},
                .imageOffset = {0U, 0U, 0U},
                .imageExtent = {.width = ImageWidth, .height = ImageHeight, .depth = 1U}};

        VkImageMemoryBarrier2 PreCopyBarrier {
                .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR,
                .pNext               = nullptr,
                .srcStageMask        = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                .srcAccessMask       = 0U,
                .dstStageMask        = VK_PIPELINE_STAGE_TRANSFER_BIT,
                .dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT,
                .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = Image,
                .subresourceRange    = {
                           .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                           .baseMipLevel   = 0U,
                           .levelCount     = 1U,
                           .baseArrayLayer = 0U,
                           .layerCount     = 1U}};

        VkImageMemoryBarrier2 PostCopyBarrier {
                .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR,
                .pNext               = nullptr,
                .srcStageMask        = VK_PIPELINE_STAGE_TRANSFER_BIT,
                .srcAccessMask       = VK_ACCESS_TRANSFER_READ_BIT,
                .dstStageMask        = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                .dstAccessMask       = 0U,
                .oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = Image,
                .subresourceRange    = {
                           .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                           .baseMipLevel   = 0U,
                           .levelCount     = 1U,
                           .baseArrayLayer = 0U,
                           .layerCount     = 1U}};

        VkDependencyInfo DependencyInfo {
                .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR,
                .imageMemoryBarrierCount = 1U,
                .pImageMemoryBarriers    = &PreCopyBarrier};

        vkCmdPipelineBarrier2(CommandBuffer.back(), &DependencyInfo);
        vkCmdCopyImageToBuffer(CommandBuffer.back(), Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Buffer, 1U, &Region);

        DependencyInfo.pImageMemoryBarriers = &PostCopyBarrier;
        vkCmdPipelineBarrier2(CommandBuffer.back(), &DependencyInfo);
    }
    FinishSingleCommandQueue(Queue, CommandPool, CommandBuffer);

    void* ImageData;
    vmaMapMemory(m_Allocator, Allocation, &ImageData);

    auto ImagePixels = static_cast<unsigned char*>(ImageData);
    for (std::uint32_t Iterator = 0; Iterator < ImageWidth * ImageHeight; ++Iterator)
    {
        std::swap(ImagePixels[Iterator * Components], ImagePixels[Iterator * Components + 2U]);
    }

    stbi_write_png(std::data(Path), ImageWidth, ImageHeight, Components, ImagePixels, ImageWidth * Components);

    vmaUnmapMemory(m_Allocator, Allocation);
    vmaDestroyBuffer(m_Allocator, Buffer, Allocation);
}

VmaAllocator const& BufferManager::GetAllocator() const
{
    return m_Allocator;
}
