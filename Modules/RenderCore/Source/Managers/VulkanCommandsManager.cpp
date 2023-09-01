// Copyright Notice: [...]

#include "Managers/VulkanCommandsManager.h"
#include <boost/log/trivial.hpp>

using namespace RenderCore;

VulkanCommandsManager::VulkanCommandsManager(const VkDevice& Device)
    : m_Device(Device)
    , m_CommandPool(VK_NULL_HANDLE)
    , m_CommandBuffers({})
    , m_ImageAvailableSemaphores({})
    , m_RenderFinishedSemaphores({})
    , m_Fences({})
    , m_ProcessingUnitsCount(0u)
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

    const std::vector<VkSubmitInfo> SubmitInfo {
        VkSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = static_cast<std::uint32_t>(m_CommandBuffers.size()),
            .pCommandBuffers = m_CommandBuffers.data()
        }
    };

    WaitAndResetFences();

    vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<std::uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());
    m_CommandBuffers.clear();

    vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
    m_CommandPool = VK_NULL_HANDLE;

    DestroySynchronizationObjects();
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

    const VkFenceCreateInfo FenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    m_ProcessingUnitsCount = static_cast<std::uint32_t>(m_CommandBuffers.size());

    m_ImageAvailableSemaphores.resize(m_ProcessingUnitsCount, VK_NULL_HANDLE);
    m_RenderFinishedSemaphores.resize(m_ProcessingUnitsCount, VK_NULL_HANDLE);
    m_Fences.resize(m_ProcessingUnitsCount, VK_NULL_HANDLE);

    for (std::uint32_t Iterator = 0u; Iterator < m_ProcessingUnitsCount; ++Iterator)
    {
        if (vkCreateSemaphore(m_Device, &SemaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphores[Iterator]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan semaphore: Image Available.");
        }

        if (vkCreateSemaphore(m_Device, &SemaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphores[Iterator]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan semaphore: Render Finished");
        }

        if (vkCreateFence(m_Device, &FenceCreateInfo, nullptr, &m_Fences[Iterator]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan fence.");
        }
    }
}

void VulkanCommandsManager::DestroySynchronizationObjects()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying Vulkan synchronization objects";

    for (const VkSemaphore& SemaphoreIter : m_ImageAvailableSemaphores)
    {
        if (SemaphoreIter != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(m_Device, SemaphoreIter, nullptr);
        }
    }
    m_ImageAvailableSemaphores.clear();

    for (const VkSemaphore& SemaphoreIter : m_RenderFinishedSemaphores)
    {
        if (SemaphoreIter != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(m_Device, SemaphoreIter, nullptr);
        }
    }
    m_RenderFinishedSemaphores.clear();

    for (const VkFence& FenceITer : m_Fences)
    {
        if (FenceITer != VK_NULL_HANDLE)
        {
            vkDestroyFence(m_Device, FenceITer, nullptr);
        }
    }
    m_Fences.clear();
}

std::vector<std::uint32_t> VulkanCommandsManager::DrawFrame(const std::vector<VkSwapchainKHR>& SwapChains)
{
    if (m_Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan logical device is invalid.");
    }

    WaitAndResetFences();

    std::vector<std::uint32_t> ImageIndexes;
    for (std::uint32_t Iterator = 0u; Iterator < m_ProcessingUnitsCount; ++Iterator)
    {
        if (m_ImageAvailableSemaphores[Iterator] == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan semaphore: Image Available is invalid.");
        }

        if (m_Fences[Iterator] == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan fence is invalid.");
        }

        if (SwapChains[Iterator] == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan swap chain is invalid.");
        }

        ImageIndexes.emplace_back(0u);
        if (const VkResult OperationResult = vkAcquireNextImageKHR(m_Device, SwapChains[Iterator], Timeout, m_ImageAvailableSemaphores[Iterator], m_Fences[Iterator], &ImageIndexes.back()); OperationResult != VK_SUCCESS)
        {
            if (OperationResult == VK_ERROR_OUT_OF_DATE_KHR)
            {
                BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Failed to acquire next image: Vulkan swap chain is outdated";
                
                vkDeviceWaitIdle(m_Device);
                DestroySynchronizationObjects();
                CreateSynchronizationObjects();

                return std::vector<std::uint32_t>();
            }
            else if (OperationResult != VK_SUBOPTIMAL_KHR)
            {
                throw std::runtime_error("Failed to acquire Vulkan swap chain image.");
            }
        }
    }

    return ImageIndexes;
}

void VulkanCommandsManager::RecordCommandBuffers(const VkRenderPass& RenderPass, const VkPipeline& Pipeline, const VkExtent2D& Extent, const std::vector<VkFramebuffer>& FrameBuffers, const std::vector<VkBuffer>& VertexBuffers, const std::vector<VkDeviceSize>& Offsets)
{
    if (Pipeline == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan graphics pipeline is invalid");
    }

    const VkCommandBufferBeginInfo CommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
    };

    const VkClearValue ClearValue{
        .color = { 0.0f, 0.0f, 0.0f, 1.0f }
    };

    auto CommandBufferIter = m_CommandBuffers.begin();
    for (const VkFramebuffer& FrameBufferIter : FrameBuffers)
    {
        if (CommandBufferIter == m_CommandBuffers.end())
        {
            break;
        }

        if (vkResetCommandBuffer(*CommandBufferIter, 0) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to end Vulkan command buffer.");
        }

        if (vkBeginCommandBuffer(*CommandBufferIter, &CommandBufferBeginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin Vulkan command buffer.");
        }

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

    WaitAndResetFences();

    const std::vector<VkPipelineStageFlags> WaitStages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    const std::vector<VkSubmitInfo> SubmitInfo{
        VkSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = static_cast<std::uint32_t>(m_ImageAvailableSemaphores.size()),
            .pWaitSemaphores = m_ImageAvailableSemaphores.data(),
            .pWaitDstStageMask = WaitStages.data(),
            .commandBufferCount = static_cast<std::uint32_t>(m_CommandBuffers.size()),
            .pCommandBuffers = m_CommandBuffers.data(),
            .signalSemaphoreCount = static_cast<std::uint32_t>(m_RenderFinishedSemaphores.size()),
            .pSignalSemaphores = m_RenderFinishedSemaphores.data()
        }
    };

    for (const VkFence& FenceIter : m_Fences)
    {
        if (FenceIter == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Vulkan fence is invalid.");
        }

        if (vkQueueSubmit(GraphicsQueue, static_cast<std::uint32_t>(SubmitInfo.size()), SubmitInfo.data(), FenceIter) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to submit Vulkan queue.");
        }
    }
}

void VulkanCommandsManager::PresentFrame(const VkQueue& PresentQueue, const std::vector<VkSwapchainKHR>& SwapChains, const std::vector<std::uint32_t>& ImageIndexes)
{
    if (PresentQueue == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan present queue is invalid.");
    }

    const VkPresentInfoKHR PresentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = static_cast<std::uint32_t>(m_RenderFinishedSemaphores.size()),
        .pWaitSemaphores = m_RenderFinishedSemaphores.data(),
        .swapchainCount = static_cast<std::uint32_t>(SwapChains.size()),
        .pSwapchains = SwapChains.data(),
        .pImageIndices = ImageIndexes.data(),
        .pResults = nullptr
    };

    if (const VkResult OperationResult = vkQueuePresentKHR(PresentQueue, &PresentInfo); OperationResult != VK_SUCCESS && OperationResult != VK_SUBOPTIMAL_KHR)
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

void VulkanCommandsManager::WaitAndResetFences()
{
    if (vkWaitForFences(m_Device, static_cast<std::uint32_t>(m_Fences.size()), m_Fences.data(), VK_TRUE, Timeout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to wait Vulkan fence.");
    }

    if (vkResetFences(m_Device, static_cast<std::uint32_t>(m_Fences.size()), m_Fences.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to reset Vulkan fence.");
    }
}
