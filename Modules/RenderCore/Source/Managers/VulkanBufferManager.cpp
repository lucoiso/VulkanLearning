// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "Managers/VulkanBufferManager.h"
#include "Utils/RenderCoreHelpers.h"
#include <boost/log/trivial.hpp>
#include <chrono>

#ifndef VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#endif
#include "vk_mem_alloc.h"

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>
#include "VulkanBufferManager.h"

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
        , m_SwapChainImageMemory({})
        , m_FrameBuffers({})
        , m_VertexBuffers({})
        , m_VertexBuffersMemory({})
        , m_IndexBuffers({})
        , m_IndexBuffersMemory({})
        , m_UniformBuffers({})
        , m_UniformBuffersMemory({})
        , m_UniformBuffersData({})
        , m_Vertices({
            Vertex{
                .Position = glm::vec2(-1.f, -1.f),
                .Color = glm::vec3(1.f, 0.f, 0.f)},
            Vertex{
                .Position = glm::vec2(1.f, -1.f),
                .Color = glm::vec3(0.f, 1.f, 0.f)},
            Vertex{
                .Position = glm::vec2(1.f, 1.f),
                .Color = glm::vec3(0.f, 0.f, 1.f)},
            Vertex{
                .Position = glm::vec2(-1.f, 1.f),
                .Color = glm::vec3(1.f, 1.f, 1.f)}})
        , m_Indices(
            {0u, 1u, 2u, 2u, 3u, 0u})
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan buffer manager";

        CreateTriangle(m_Vertices, m_Indices);
        // CreateSquare(m_Vertices, m_Indices);
        // CreateCircle(m_Vertices, m_Indices);
        // CreateSphere(m_Vertices, m_Indices);
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

        const VkSwapchainCreateInfoKHR SwapChainCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = m_Surface,
            .minImageCount = Capabilities.minImageCount,
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

        // AllocateSwapChainImages(PreferredFormat.format, PreferredExtent);
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
            const VkFramebufferCreateInfo FrameBufferCreateInfo{
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = RenderPass,
                .attachmentCount = 1u,
                .pAttachments = &m_SwapChainImageViews[Iterator],
                .width = Extent.width,
                .height = Extent.height,
                .layers = 1u};

            RENDERCORE_CHECK_VULKAN_RESULT(vkCreateFramebuffer(m_Device, &FrameBufferCreateInfo, nullptr, &m_FrameBuffers[Iterator]));
        }
    }

    void CreateVertexBuffers(const VkPhysicalDevice &PhysicalDevice, const VkQueue &TransferQueue, const VkCommandPool &CommandPool)
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
        vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemoryProperties);

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
            CopyBuffer(StagingBuffer, m_VertexBuffers[Iterator], BufferSize, TransferQueue, CommandPool);

            vmaDestroyBuffer(m_Allocator, StagingBuffer, StagingBufferMemory);
            // vmaFreeMemory(m_Allocator, StagingBufferMemory);
        }
    }

    void CreateIndexBuffers(const VkPhysicalDevice &PhysicalDevice, const VkQueue &TransferQueue, const VkCommandPool &CommandPool)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan index buffers";

        if (m_Allocator == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan memory allocator is invalid.");
        }

        m_IndexBuffers.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);
        m_IndexBuffersMemory.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);

        const VkDeviceSize BufferSize = m_Indices.size() * sizeof(std::uint16_t);

        VkPhysicalDeviceMemoryProperties MemoryProperties;
        vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemoryProperties);

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
            CopyBuffer(StagingBuffer, m_IndexBuffers[Iterator], BufferSize, TransferQueue, CommandPool);

            vmaDestroyBuffer(m_Allocator, StagingBuffer, StagingBufferMemory);
            // vmaFreeMemory(m_Allocator, StagingBufferMemory);
        }
    }

    void CreateUniformBuffers(const VkPhysicalDevice &PhysicalDevice, const VkQueue &TransferQueue)
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
        vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &MemoryProperties);

        for (std::uint32_t Iterator = 0u; Iterator < g_MaxFramesInFlight; ++Iterator)
        {
            CreateBuffer(MemoryProperties, BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_UniformBuffers[Iterator], m_UniformBuffersMemory[Iterator]);
            vmaMapMemory(m_Allocator, m_UniformBuffersMemory[Iterator], &m_UniformBuffersData[Iterator]);
        }
    }
    
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

    void CreateBuffer(const VkPhysicalDeviceMemoryProperties &Properties, const VkDeviceSize &Size, const VkBufferUsageFlags &Usage, const VkMemoryPropertyFlags &Flags, VkBuffer &Buffer, VmaAllocation &Allocation) const
    {
        if (m_Allocator == nullptr || m_Allocator == VK_NULL_HANDLE)
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
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        const VmaAllocationCreateInfo AllocationCreateInfo{
            .usage = VMA_MEMORY_USAGE_GPU_ONLY,
            .requiredFlags = Flags | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        };

        VmaAllocationInfo MemoryAllocationInfo;
        RENDERCORE_CHECK_VULKAN_RESULT(vmaCreateBuffer(m_Allocator, &BufferCreateInfo, &AllocationCreateInfo, &Buffer, &Allocation, &MemoryAllocationInfo));
    }

    void CopyBuffer(const VkBuffer &Source, const VkBuffer &Destination, const VkDeviceSize &Size, const VkQueue &TransferQueue, const VkCommandPool &CommandPool) const
    {
        if (m_Device == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan logical device is invalid.");
        }

        if (CommandPool == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan command pool is invalid.");
        }

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

        VkCommandBuffer CommandBuffer;
        vkAllocateCommandBuffers(m_Device, &CommandBufferAllocateInfo, &CommandBuffer);

        RENDERCORE_CHECK_VULKAN_RESULT(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));

        const VkBufferCopy BufferCopy{
            .size = Size,
        };

        vkCmdCopyBuffer(CommandBuffer, Source, Destination, 1u, &BufferCopy);    
        RENDERCORE_CHECK_VULKAN_RESULT(vkEndCommandBuffer(CommandBuffer));

        const VkSubmitInfo SubmitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1u,
            .pCommandBuffers = &CommandBuffer,
        };

        RENDERCORE_CHECK_VULKAN_RESULT(vkQueueSubmit(TransferQueue, 1u, &SubmitInfo, VK_NULL_HANDLE));
        RENDERCORE_CHECK_VULKAN_RESULT(vkQueueWaitIdle(TransferQueue));

        vkFreeCommandBuffers(m_Device, CommandPool, 1u, &CommandBuffer);
        vkDestroyCommandPool(m_Device, CommandPool, nullptr);
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

    void Shutdown()
    {
        if (!IsInitialized())
        {
            return;
        }

        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down Vulkan buffer manager";
        DestroyResources();

        for (std::uint32_t Iterator = 0u; Iterator < static_cast<std::uint32_t>(m_VertexBuffers.size()); ++Iterator)
        {
            // vmaUnmapMemory(m_Allocator, m_VertexBuffersMemory[Iterator]);
            vmaDestroyBuffer(m_Allocator, m_VertexBuffers[Iterator], m_VertexBuffersMemory[Iterator]);
            // vmaFreeMemory(m_Allocator, m_VertexBuffersMemory[Iterator]);
        }
        m_VertexBuffers.clear();
        m_VertexBuffersMemory.clear();

        for (std::uint32_t Iterator = 0u; Iterator < static_cast<std::uint32_t>(m_IndexBuffers.size()); ++Iterator)
        {
            // vmaUnmapMemory(m_Allocator, m_IndexBuffersMemory[Iterator]);
            vmaDestroyBuffer(m_Allocator, m_IndexBuffers[Iterator], m_IndexBuffersMemory[Iterator]);
            // vmaFreeMemory(m_Allocator, m_IndexBuffersMemory[Iterator]);
        }
        m_IndexBuffers.clear();
        m_IndexBuffersMemory.clear();

        for (std::uint32_t Iterator = 0u; Iterator < static_cast<std::uint32_t>(m_UniformBuffers.size()); ++Iterator)
        {
            vmaUnmapMemory(m_Allocator, m_UniformBuffersMemory[Iterator]);
            vmaDestroyBuffer(m_Allocator, m_UniformBuffers[Iterator], m_UniformBuffersMemory[Iterator]);
            // vmaFreeMemory(m_Allocator, m_UniformBuffersMemory[Iterator]);
        }
        m_UniformBuffers.clear();
        m_UniformBuffersMemory.clear();
        m_UniformBuffersData.clear();

        vmaDestroyAllocator(m_Allocator);
        m_Allocator = VK_NULL_HANDLE;
    }

    bool IsInitialized() const
    {
        return m_Device != VK_NULL_HANDLE && m_Surface != VK_NULL_HANDLE && m_SwapChain != VK_NULL_HANDLE;
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

    const std::vector<std::uint16_t> &GetIndices() const
    {
        return m_Indices;
    }

    std::uint32_t GetIndexCount() const
    {
        return static_cast<std::uint32_t>(m_Indices.size());
    }

private:
    void AllocateSwapChainImages(const VkFormat &ImageFormat, const VkExtent2D& Extent)
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan image views";

        if (m_Device == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan logical device is invalid.");
        }

        m_SwapChainImageMemory.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);
        for (auto ImageIter = m_SwapChainImages.begin(); ImageIter != m_SwapChainImages.end(); ++ImageIter)
        {
            const VkImageCreateInfo ImageViewCreateInfo{
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = ImageFormat,
                .extent = { .width = Extent.width, .height = Extent.height, .depth = 1u},
                .mipLevels = 1u,
                .arrayLayers = 1u,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};

            const VmaAllocationCreateInfo ImageAllocationInfo {
                .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
                .usage = VMA_MEMORY_USAGE_GPU_ONLY,
                .priority = 1.f};

            const std::int32_t Index = static_cast<std::uint32_t>(std::distance(m_SwapChainImages.begin(), ImageIter));
            RENDERCORE_CHECK_VULKAN_RESULT(vmaCreateImage(m_Allocator, &ImageViewCreateInfo, &ImageAllocationInfo, &*ImageIter, &m_SwapChainImageMemory[Index], nullptr));
        }
    }

    void CreateSwapChainImageViews(const VkFormat &ImageFormat)
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
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY},
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0u,
                    .levelCount = 1u,
                    .baseArrayLayer = 0u,
                    .layerCount = 1u}};

            const std::int32_t Index = std::distance(m_SwapChainImages.begin(), ImageIter);
            RENDERCORE_CHECK_VULKAN_RESULT(vkCreateImageView(m_Device, &ImageViewCreateInfo, nullptr, &m_SwapChainImageViews[Index]))
        }
    }

    void DestroyResources()
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

        for (std::uint32_t Iterator = 0u; Iterator < static_cast<std::uint32_t>(m_SwapChainImageMemory.size()); ++Iterator)
        {
            // vmaUnmapMemory(m_Allocator, m_UniformBuffersMemory[Iterator]);
            vmaDestroyImage(m_Allocator, m_SwapChainImages[Iterator], m_SwapChainImageMemory[Iterator]);
            // vmaFreeMemory(m_Allocator, m_UniformBuffersMemory[Iterator]);
        }
        m_SwapChainImages.clear();
        m_SwapChainImageMemory.clear();

        for (const VkImageView &ImageViewIter : m_SwapChainImageViews)
        {
            if (ImageViewIter != VK_NULL_HANDLE)
            {
                vkDestroyImageView(m_Device, ImageViewIter, nullptr);
            }
        }
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

    const VkDevice &m_Device;
    const VkSurfaceKHR &m_Surface;
    const std::vector<std::uint32_t> m_QueueFamilyIndices;

    VmaAllocator m_Allocator;
    VkSwapchainKHR m_SwapChain;
    std::vector<VkImage> m_SwapChainImages;
    std::vector<VkImageView> m_SwapChainImageViews;
    std::vector<VmaAllocation> m_SwapChainImageMemory;
    std::vector<VkFramebuffer> m_FrameBuffers;
    std::vector<VkBuffer> m_VertexBuffers;
    std::vector<VmaAllocation> m_VertexBuffersMemory;
    std::vector<VkBuffer> m_IndexBuffers;
    std::vector<VmaAllocation> m_IndexBuffersMemory;
    std::vector<VkBuffer> m_UniformBuffers;
    std::vector<VmaAllocation> m_UniformBuffersMemory;
    std::vector<void *> m_UniformBuffersData;
    std::vector<Vertex> m_Vertices;
    std::vector<std::uint16_t> m_Indices;
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

void VulkanBufferManager::CreateVertexBuffers(const VkPhysicalDevice &PhysicalDevice, const VkQueue &TransferQueue, const VkCommandPool& CommandPool)
{
    m_Impl->CreateVertexBuffers(PhysicalDevice, TransferQueue, CommandPool);
}

void VulkanBufferManager::CreateIndexBuffers(const VkPhysicalDevice &PhysicalDevice, const VkQueue &TransferQueue, const VkCommandPool& CommandPool)
{
    m_Impl->CreateIndexBuffers(PhysicalDevice, TransferQueue, CommandPool);
}

void VulkanBufferManager::CreateUniformBuffers(const VkPhysicalDevice &PhysicalDevice, const VkQueue &TransferQueue)
{
    m_Impl->CreateUniformBuffers(PhysicalDevice, TransferQueue);
}

void VulkanBufferManager::UpdateUniformBuffers(const std::uint32_t Frame, const VkExtent2D &SwapChainExtent)
{
    m_Impl->UpdateUniformBuffers(Frame, SwapChainExtent);
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

const std::vector<std::uint16_t> &VulkanBufferManager::GetIndices() const
{
    return m_Impl->GetIndices();
}

std::uint32_t VulkanBufferManager::GetIndexCount() const
{
    return m_Impl->GetIndexCount();
}