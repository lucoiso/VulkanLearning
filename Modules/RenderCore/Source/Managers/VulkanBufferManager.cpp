// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "Managers/VulkanBufferManager.h"
#include "Utils/RenderCoreHelpers.h"
#include <boost/log/trivial.hpp>

using namespace RenderCore;

VulkanBufferManager::VulkanBufferManager(const VkDevice& Device, const VkSurfaceKHR& Surface, const std::vector<std::uint32_t>& QueueFamilyIndices)
    : m_Device(Device)
    , m_Surface(Surface)
    , m_QueueFamilyIndices(QueueFamilyIndices)
    , m_SwapChain(VK_NULL_HANDLE)
    , m_SwapChainImages({})
    , m_SwapChainImageViews({})
    , m_FrameBuffers({})
    , m_VertexBuffers({})
    , m_VertexBuffersMemory({})
    , m_IndexBuffers({})
    , m_IndexBuffersMemory({})
    , m_Vertices({
        Vertex {
            .Position = { -1.f, -1.f, 0.f },
            .Color = {1.0f, 0.0f, 0.0f }
        },
        Vertex {
            .Position = { 1.f, -1.f, 0.f },
            .Color = {0.0f, 1.0f, 0.0f}
        },
        Vertex {
            .Position = { 1.f, 1.f, 0.f },
            .Color = {0.0f, 0.0f, 1.0f}
        },
        Vertex {
            .Position = { -1.f, 1.f, 0.f },
            .Color = { 0.0f, 0.0f, 1.0f }
        }
    })
    , m_Indices(
        { 0u, 1u, 2u, 2u, 3u, 0u }
    )
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan buffer manager";
    CreateCircle(m_Vertices, m_Indices);
}

VulkanBufferManager::~VulkanBufferManager()
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing vulkan buffer manager";
    Shutdown();
}
void VulkanBufferManager::CreateSwapChain(const VkSurfaceFormatKHR& PreferredFormat, const VkPresentModeKHR& PreferredMode, const VkExtent2D& PreferredExtent, const VkSurfaceCapabilitiesKHR& Capabilities)
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

    const VkSwapchainCreateInfoKHR SwapChainCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = m_Surface,
        .minImageCount = Capabilities.minImageCount,
        .imageFormat = PreferredFormat.format,
        .imageColorSpace = PreferredFormat.colorSpace,
        .imageExtent = PreferredExtent,
        .imageArrayLayers = 1u,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = m_QueueFamilyIndices.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = static_cast<std::uint32_t>(m_QueueFamilyIndices.size()),
        .pQueueFamilyIndices = m_QueueFamilyIndices.data(),
        .preTransform = Capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = PreferredMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateSwapchainKHR(m_Device, &SwapChainCreateInfo, nullptr, &m_SwapChain));

    std::uint32_t Count = 0u;
    RENDERCORE_CHECK_VULKAN_RESULT(vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &Count, nullptr));

    m_SwapChainImages.resize(Count, VK_NULL_HANDLE);
    RENDERCORE_CHECK_VULKAN_RESULT(vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &Count, m_SwapChainImages.data()));

    CreateSwapChainImageViews(PreferredFormat.format);
}

void VulkanBufferManager::CreateFrameBuffers(const VkRenderPass& RenderPass, const VkExtent2D& Extent)
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

    m_FrameBuffers.resize(m_SwapChainImageViews.size(), VK_NULL_HANDLE);

    const VkFramebufferCreateInfo FrameBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = RenderPass,
        .attachmentCount = static_cast<std::uint32_t>(m_SwapChainImageViews.size()),
        .pAttachments = m_SwapChainImageViews.data(),
        .width = Extent.width,
        .height = Extent.height,
        .layers = 1u
    };

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateFramebuffer(m_Device, &FrameBufferCreateInfo, nullptr, m_FrameBuffers.data()));
}

void VulkanBufferManager::CreateVertexBuffers(const VkPhysicalDevice& PhysicalDevice, const VkQueue& TransferQueue, const std::vector<VkCommandBuffer>& CommandBuffers, const VkCommandPool& CommandPool)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan vertex buffers";

    if (m_Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan logical device is invalid.");
    }

    m_VertexBuffers.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);
    m_VertexBuffersMemory.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);

    const VkDeviceSize BufferSize = m_Vertices.size() * sizeof(Vertex);

    VkPhysicalDeviceMemoryProperties MemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemoryProperties);

    for (std::uint32_t Iterator = 0u; Iterator < static_cast<std::uint32_t>(m_VertexBuffers.size()); ++Iterator)
    {
        VkBuffer StagingBuffer;
        VkDeviceMemory StagingBufferMemory;
        CreateBuffer(MemoryProperties, BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer, StagingBufferMemory);

        void* Data;
        vkMapMemory(m_Device, StagingBufferMemory, 0u, BufferSize, 0u, &Data);
        std::memcpy(Data, m_Vertices.data(), static_cast<std::size_t>(BufferSize));
        vkUnmapMemory(m_Device, StagingBufferMemory);

        CreateBuffer(MemoryProperties, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffers[Iterator], m_VertexBuffersMemory[Iterator]);
        CopyBuffer(StagingBuffer, m_VertexBuffers[Iterator], BufferSize, TransferQueue, CommandBuffers, CommandPool);

        vkDestroyBuffer(m_Device, StagingBuffer, nullptr);
        vkFreeMemory(m_Device, StagingBufferMemory, nullptr);
    }
}

void VulkanBufferManager::CreateIndexBuffers(const VkPhysicalDevice& PhysicalDevice, const VkQueue& TransferQueue, const std::vector<VkCommandBuffer>& CommandBuffers, const VkCommandPool& CommandPool)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan index buffers";

    if (m_Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan logical device is invalid.");
    }

    m_IndexBuffers.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);
    m_IndexBuffersMemory.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);

    const VkDeviceSize BufferSize = m_Indices.size() * sizeof(std::uint32_t);

    VkPhysicalDeviceMemoryProperties MemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemoryProperties);

    for (std::uint32_t Iterator = 0u; Iterator < static_cast<std::uint32_t>(m_IndexBuffers.size()); ++Iterator)
    {
        VkBuffer StagingBuffer;
        VkDeviceMemory StagingBufferMemory;
        CreateBuffer(MemoryProperties, BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, StagingBuffer, StagingBufferMemory);

        void* Data;
        vkMapMemory(m_Device, StagingBufferMemory, 0u, BufferSize, 0u, &Data);
        std::memcpy(Data, m_Indices.data(), static_cast<std::size_t>(BufferSize));
        vkUnmapMemory(m_Device, StagingBufferMemory);

        CreateBuffer(MemoryProperties, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBuffers[Iterator], m_IndexBuffersMemory[Iterator]);
        CopyBuffer(StagingBuffer, m_IndexBuffers[Iterator], BufferSize, TransferQueue, CommandBuffers, CommandPool);

        vkDestroyBuffer(m_Device, StagingBuffer, nullptr);
        vkFreeMemory(m_Device, StagingBufferMemory, nullptr);
    }
}

void VulkanBufferManager::CreateBuffer(const VkPhysicalDeviceMemoryProperties& Properties, const VkDeviceSize& Size, const VkBufferUsageFlags& Usage, const VkMemoryPropertyFlags& Flags, VkBuffer& Buffer, VkDeviceMemory& BufferMemory) const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan buffer";

    if (m_Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan logical device is invalid.");
    }

    const VkBufferCreateInfo BufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = Size,
        .usage = Usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateBuffer(m_Device, &BufferCreateInfo, nullptr, &Buffer));

    VkMemoryRequirements MemoryRequirements;
    vkGetBufferMemoryRequirements(m_Device, Buffer, &MemoryRequirements);

    const VkMemoryAllocateInfo MemoryAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = MemoryRequirements.size,
        .memoryTypeIndex = FindMemoryType(Properties, MemoryRequirements.memoryTypeBits, Flags),
    };

    RENDERCORE_CHECK_VULKAN_RESULT(vkAllocateMemory(m_Device, &MemoryAllocateInfo, nullptr, &BufferMemory));
    RENDERCORE_CHECK_VULKAN_RESULT(vkBindBufferMemory(m_Device, Buffer, BufferMemory, 0u));
}

void VulkanBufferManager::CopyBuffer(const VkBuffer& Source, const VkBuffer& Destination, const VkDeviceSize& Size, const VkQueue& TransferQueue, const std::vector<VkCommandBuffer>& CommandBuffers, const VkCommandPool& CommandPool) const
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Copying Vulkan buffer";

    if (m_Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan logical device is invalid.");
    }

    if (CommandBuffers.empty())
    {
        throw std::runtime_error("Vulkan command buffers are empty.");
    }

    if (CommandPool == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan command pool is invalid.");
    }

    const VkCommandBufferBeginInfo CommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };


    for (const VkCommandBuffer& CommandBufferIter : CommandBuffers)
    {
        RENDERCORE_CHECK_VULKAN_RESULT(vkBeginCommandBuffer(CommandBuffers[0], &CommandBufferBeginInfo));

        const VkBufferCopy BufferCopy{
            .srcOffset = 0u,
            .dstOffset = 0u,
            .size = Size,
        };

        vkCmdCopyBuffer(CommandBufferIter, Source, Destination, 1u, &BufferCopy);

        RENDERCORE_CHECK_VULKAN_RESULT(vkEndCommandBuffer(CommandBufferIter));
    }

    const VkSubmitInfo SubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = static_cast<std::uint32_t>(CommandBuffers.size()),
        .pCommandBuffers = CommandBuffers.data(),
    };

    RENDERCORE_CHECK_VULKAN_RESULT(vkQueueSubmit(TransferQueue, 1u, &SubmitInfo, VK_NULL_HANDLE));
    RENDERCORE_CHECK_VULKAN_RESULT(vkQueueWaitIdle(TransferQueue));
}

void VulkanBufferManager::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down Vulkan buffer manager";
    DestroyResources();
}

bool VulkanBufferManager::IsInitialized() const
{
    return m_Device     != VK_NULL_HANDLE
        && m_Surface    != VK_NULL_HANDLE
        && m_SwapChain  != VK_NULL_HANDLE;
}

const VkSwapchainKHR& VulkanBufferManager::GetSwapChain() const
{
    return m_SwapChain;
}

const std::vector<VkImage>& VulkanBufferManager::GetSwapChainImages() const
{
    return m_SwapChainImages;
}

const std::vector<VkImageView>& VulkanBufferManager::GetSwapChainImageViews() const
{
    return m_SwapChainImageViews;
}

const std::vector<VkFramebuffer>& VulkanBufferManager::GetFrameBuffers() const
{
    return m_FrameBuffers;
}

const std::vector<VkBuffer>& VulkanBufferManager::GetVertexBuffers() const
{
    return m_VertexBuffers;
}

const std::vector<VkDeviceMemory>& VulkanBufferManager::GetVertexBuffersMemory() const
{
    return m_VertexBuffersMemory;
}

const std::vector<VkBuffer>& VulkanBufferManager::GetIndexBuffers() const
{
    return m_IndexBuffers;
}

const std::vector<VkDeviceMemory>& VulkanBufferManager::GetIndexBuffersMemory() const
{
    return m_IndexBuffersMemory;
}

const std::vector<Vertex>& VulkanBufferManager::GetVertices() const
{
    return m_Vertices;
}

const std::vector<std::uint32_t>& VulkanBufferManager::GetIndices() const
{
    return m_Indices;
}

std::uint32_t VulkanBufferManager::GetIndexCount() const
{
    return static_cast<std::uint32_t>(m_Indices.size());
}

void VulkanBufferManager::CreateSwapChainImageViews(const VkFormat& ImageFormat)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan image views";

    if (m_Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan logical device is invalid.");
    }

    m_SwapChainImageViews.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);
    for (auto ImageIter = m_SwapChainImages.begin(); ImageIter != m_SwapChainImages.end(); ++ImageIter)
    {
        const VkImageViewCreateInfo ImageViewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = *ImageIter,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = ImageFormat,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0u,
                .levelCount = 1u,
                .baseArrayLayer = 0u,
                .layerCount = 1u
            }
        };
        
        const std::int32_t Index = std::distance(m_SwapChainImages.begin(), ImageIter);
        RENDERCORE_CHECK_VULKAN_RESULT(vkCreateImageView(m_Device, &ImageViewCreateInfo, nullptr, &m_SwapChainImageViews[Index]))
    }
}

void VulkanBufferManager::DestroyResources()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying resources from vulkan buffer manager";

    if (m_Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan logical device is invalid.");
    }

    if (m_SwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
        m_SwapChain = VK_NULL_HANDLE;
    }

    for (const VkImageView& ImageViewIter : m_SwapChainImageViews)
    {
        if (ImageViewIter != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_Device, ImageViewIter, nullptr);
        }
    }
    m_SwapChainImageViews.clear();
    m_SwapChainImages.clear();

    for (const VkFramebuffer& FrameBufferIter : m_FrameBuffers)
    {
        if (FrameBufferIter != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(m_Device, FrameBufferIter, nullptr);
        }
    }
    m_FrameBuffers.clear();

    for (const VkBuffer& VertexBufferIter : m_VertexBuffers)
    {
        if (VertexBufferIter != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(m_Device, VertexBufferIter, nullptr);
        }
    }
    m_VertexBuffers.clear();

    for (const VkDeviceMemory& VertexBufferMemoryIter : m_VertexBuffersMemory)
    {
        if (VertexBufferMemoryIter != VK_NULL_HANDLE)
        {
            vkFreeMemory(m_Device, VertexBufferMemoryIter, nullptr);
        }
    }
    m_VertexBuffersMemory.clear();

    for (const VkBuffer& IndexBufferIter : m_IndexBuffers)
    {
        if (IndexBufferIter != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(m_Device, IndexBufferIter, nullptr);
        }
    }
    m_IndexBuffers.clear();

    for (const VkDeviceMemory& IndexBufferMemoryIter : m_IndexBuffersMemory)
    {
        if (IndexBufferMemoryIter != VK_NULL_HANDLE)
        {
            vkFreeMemory(m_Device, IndexBufferMemoryIter, nullptr);
        }
    }
    m_IndexBuffersMemory.clear();
}

std::uint32_t VulkanBufferManager::FindMemoryType(const VkPhysicalDeviceMemoryProperties& Properties, const std::uint32_t& TypeFilter, const VkMemoryPropertyFlags& Flags) const
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
