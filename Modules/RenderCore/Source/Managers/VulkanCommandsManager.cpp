// Copyright Notice: [...]

#include "Managers/VulkanCommandsManager.h"
#include <boost/log/trivial.hpp>

using namespace RenderCore;

VulkanCommandsManager::VulkanCommandsManager(const VkDevice& Device)
    : m_Device(Device)
    , m_CommandPool(VK_NULL_HANDLE)
    , m_CommandBuffers({})
    , m_ImageAvailableSemaphore(VK_NULL_HANDLE)
    , m_RenderFinishedSemaphore(VK_NULL_HANDLE)
    , m_Fence(VK_NULL_HANDLE)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan commands manager";
}

VulkanCommandsManager::~VulkanCommandsManager()
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing vulkan commands manager";
    Shutdown({});
}

void VulkanCommandsManager::Shutdown(const std::vector<VkQueue>& PendingQueues)
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down Vulkan commands manager";

    const VkSubmitInfo SubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = static_cast<std::uint32_t>(m_CommandBuffers.size()),
        .pCommandBuffers = m_CommandBuffers.data()
    };

    for (const VkQueue& QueueIter : PendingQueues)
    {
        if (vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, std::numeric_limits<std::uint64_t>::max()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to wait Vulkan fence.");
        }

        if (vkResetFences(m_Device, 1, &m_Fence) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to reset Vulkan fence.");
        }

        if (vkQueueSubmit(QueueIter, 1, &SubmitInfo, m_Fence) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to submit Vulkan queue.");
        }

        if (vkQueueWaitIdle(QueueIter) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to wait Vulkan queue idle.");
        }

    }

    vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<std::uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());
    m_CommandBuffers.clear();

    vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
    m_CommandPool = VK_NULL_HANDLE;

    vkDestroySemaphore(m_Device, m_ImageAvailableSemaphore, nullptr);
    m_ImageAvailableSemaphore = VK_NULL_HANDLE;

    vkDestroySemaphore(m_Device, m_RenderFinishedSemaphore, nullptr);
    m_RenderFinishedSemaphore = VK_NULL_HANDLE;

    vkDestroyFence(m_Device, m_Fence, nullptr);
    m_Fence = VK_NULL_HANDLE;
}

void VulkanCommandsManager::CreateCommandPool(const std::vector<VkFramebuffer>& FrameBuffers, const std::uint32_t GraphicsFamilyQueueIndex)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan command pool";

    if (m_Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan logical device is invalid.");
    }

    const VkCommandPoolCreateInfo CommandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = GraphicsFamilyQueueIndex
    };

    if (vkCreateCommandPool(m_Device, &CommandPoolCreateInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan command pool.");
    }

    const VkCommandBufferAllocateInfo CommandBufferAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_CommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<std::uint32_t>(FrameBuffers.size())
    };

    m_CommandBuffers.resize(FrameBuffers.size(), VK_NULL_HANDLE);
    if (vkAllocateCommandBuffers(m_Device, &CommandBufferAllocateInfo, m_CommandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate Vulkan command buffers.");
    }
}

void VulkanCommandsManager::CreateSynchronizationObjects()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan synchronization objects";

    if (m_Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan logical device is invalid.");
    }

    const VkSemaphoreCreateInfo SemaphoreCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    if (vkCreateSemaphore(m_Device, &SemaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphore) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan semaphore: Image Available.");
    }

    if (vkCreateSemaphore(m_Device, &SemaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphore) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan semaphore: Render Finished");
    }

    const VkFenceCreateInfo FenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    if (vkCreateFence(m_Device, &FenceCreateInfo, nullptr, &m_Fence) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan fence.");
    }
}

std::uint32_t VulkanCommandsManager::DrawFrame(const VkSwapchainKHR& SwapChain)
{
    if (m_Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan logical device is invalid.");
    }

    if (SwapChain == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan swap chain is invalid.");
    }

    if (vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, std::numeric_limits<std::uint64_t>::max()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to wait Vulkan fence.");
    }

    if (vkResetFences(m_Device, 1, &m_Fence) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to reset Vulkan fence.");
    }

    std::uint32_t ImageIndex;
    if (vkAcquireNextImageKHR(m_Device, SwapChain, std::numeric_limits<std::uint64_t>::max(), m_ImageAvailableSemaphore, VK_NULL_HANDLE, &ImageIndex) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to acquire Vulkan swap chain image.");
    }

    return ImageIndex;
}

void VulkanCommandsManager::RecordCommandBuffers(const VkRenderPass& RenderPass, const VkPipeline& Pipeline, const VkExtent2D& Extent, const std::vector<VkFramebuffer>& FrameBuffers, const std::vector<VkBuffer>& VertexBuffers, const std::vector<VkDeviceSize>& Offsets)
{
    if (Pipeline == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan graphics pipeline is invalid");
    }

    auto CommandBufferIter = m_CommandBuffers.begin();
    for (const VkFramebuffer& FrameBufferIter : FrameBuffers)
    {
        if (CommandBufferIter == m_CommandBuffers.end())
        {
            break;
        }

        const VkCommandBufferBeginInfo CommandBufferBeginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
        };

        if (vkResetCommandBuffer(*CommandBufferIter, 0) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to end Vulkan command buffer.");
        }

        if (vkBeginCommandBuffer(*CommandBufferIter, &CommandBufferBeginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin Vulkan command buffer.");
        }

        const VkClearValue ClearValue{
            .color = { 0.0f, 0.0f, 0.0f, 1.0f }
        };

        const VkRenderPassBeginInfo RenderPassBeginInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = RenderPass,
            .framebuffer = FrameBufferIter,
            .renderArea = {
                .offset = { 0, 0 },
                .extent = Extent
            },
            .clearValueCount = 1u,
            .pClearValues = &ClearValue
        };

        vkCmdBeginRenderPass(*CommandBufferIter, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(*CommandBufferIter, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);

        // vkCmdBindVertexBuffers(*CommandBufferIter, 0, 1, VertexBuffer.data(), Offsets.data());

        // vkCmdSetViewport(*CommandBufferIter, 0u, 1u, &m_Viewport);
        // vkCmdSetScissor(*CommandBufferIter, 0u, 1u, &m_Scissor);

        vkCmdDraw(*CommandBufferIter, 3, 1, 0, 0);
        vkCmdEndRenderPass(*CommandBufferIter);

        if (vkEndCommandBuffer(*CommandBufferIter) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer");
        }

        ++CommandBufferIter;
    }
}

void VulkanCommandsManager::SubmitCommandBuffers(const VkQueue& GraphicsQueue)
{
    if (GraphicsQueue == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan graphics queue is invalid.");
    }

    const std::vector<VkPipelineStageFlags> WaitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    const VkSubmitInfo SubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1u,
        .pWaitSemaphores = &m_ImageAvailableSemaphore,
        .pWaitDstStageMask = WaitStages.data(),
        .commandBufferCount = static_cast<std::uint32_t>(m_CommandBuffers.size()),
        .pCommandBuffers = m_CommandBuffers.data(),
        .signalSemaphoreCount = 1u,
        .pSignalSemaphores = &m_RenderFinishedSemaphore
    };

    if (vkQueueSubmit(GraphicsQueue, 1u, &SubmitInfo, m_Fence) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit Vulkan queue.");
    }
}

void VulkanCommandsManager::PresentFrame(const VkQueue& PresentQueue, const VkSwapchainKHR& SwapChain, const std::uint32_t ImageIndex)
{
    if (PresentQueue == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan present queue is invalid.");
    }

    if (SwapChain == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan swap chain is invalid.");
    }

    const VkPresentInfoKHR PresentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1u,
        .pWaitSemaphores = &m_RenderFinishedSemaphore,
        .swapchainCount = 1u,
        .pSwapchains = &SwapChain,
        .pImageIndices = &ImageIndex,
        .pResults = nullptr
    };

    if (vkQueuePresentKHR(PresentQueue, &PresentInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to present Vulkan queue.");
    }
}

bool VulkanCommandsManager::IsInitialized() const
{
    return m_CommandPool != VK_NULL_HANDLE;
}

const VkCommandPool& VulkanCommandsManager::GetCommandPool() const
{
    return m_CommandPool;
}

const std::vector<VkCommandBuffer>& VulkanCommandsManager::GetCommandBuffers() const
{
    return m_CommandBuffers;
}