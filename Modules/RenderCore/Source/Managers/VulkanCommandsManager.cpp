// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

#include "Managers/VulkanCommandsManager.h"
#include "Managers/VulkanBufferManager.h"
#include "Managers/VulkanDeviceManager.h"
#include "Managers/VulkanPipelineManager.h"
#include "Utils/RenderCoreHelpers.h"
#include "Utils/VulkanConstants.h"
#include <boost/log/trivial.hpp>

using namespace RenderCore;

VulkanCommandsManager::VulkanCommandsManager()
    : m_CommandPool(VK_NULL_HANDLE),
      m_CommandBuffer(VK_NULL_HANDLE),
      m_ImageAvailableSemaphore(VK_NULL_HANDLE),
      m_RenderFinishedSemaphore(VK_NULL_HANDLE),
      m_Fence(VK_NULL_HANDLE),
      m_SynchronizationObjectsCreated(false),
      m_FrameIndex(0U)
{
}

VulkanCommandsManager::~VulkanCommandsManager()
{
    try
    {
        Shutdown();
    }
    catch (...)
    {
    }
}

VulkanCommandsManager& VulkanCommandsManager::Get()
{
    static VulkanCommandsManager Instance {};
    return Instance;
}

void VulkanCommandsManager::Shutdown()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down Vulkan commands manager";

    WaitAndResetFences();

    VkDevice const& VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

    if (m_CommandBuffer != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(VulkanLogicalDevice, m_CommandPool, 1U, &m_CommandBuffer);
        m_CommandBuffer = VK_NULL_HANDLE;
    }

    if (m_CommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(VulkanLogicalDevice, m_CommandPool, nullptr);
        m_CommandPool = VK_NULL_HANDLE;
    }

    DestroySynchronizationObjects();
}

VkCommandPool VulkanCommandsManager::CreateCommandPool(std::uint8_t const FamilyQueueIndex)
{
    VkCommandPoolCreateInfo const CommandPoolCreateInfo {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = static_cast<std::uint32_t>(FamilyQueueIndex)};

    VkCommandPool Output = VK_NULL_HANDLE;
    RenderCoreHelpers::CheckVulkanResult(vkCreateCommandPool(VulkanDeviceManager::Get().GetLogicalDevice(), &CommandPoolCreateInfo, nullptr, &Output));

    return Output;
}

void VulkanCommandsManager::CreateSynchronizationObjects()
{
    if (m_SynchronizationObjectsCreated)
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan synchronization objects";

    constexpr VkSemaphoreCreateInfo SemaphoreCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    constexpr VkFenceCreateInfo FenceCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT};

    VkDevice const& VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

    RenderCoreHelpers::CheckVulkanResult(vkCreateSemaphore(VulkanLogicalDevice, &SemaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphore));
    RenderCoreHelpers::CheckVulkanResult(vkCreateSemaphore(VulkanLogicalDevice, &SemaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphore));
    RenderCoreHelpers::CheckVulkanResult(vkCreateFence(VulkanLogicalDevice, &FenceCreateInfo, nullptr, &m_Fence));

    m_SynchronizationObjectsCreated = true;
}

void VulkanCommandsManager::DestroySynchronizationObjects()
{
    if (!m_SynchronizationObjectsCreated)
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying Vulkan synchronization objects";

    VkDevice const& VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

    vkDeviceWaitIdle(VulkanLogicalDevice);

    if (m_CommandBuffer != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(VulkanLogicalDevice, m_CommandPool, 1U, &m_CommandBuffer);
        m_CommandBuffer = VK_NULL_HANDLE;
    }

    if (m_CommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(VulkanLogicalDevice, m_CommandPool, nullptr);
        m_CommandPool = VK_NULL_HANDLE;
    }

    if (m_ImageAvailableSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(VulkanLogicalDevice, m_ImageAvailableSemaphore, nullptr);
        m_ImageAvailableSemaphore = VK_NULL_HANDLE;
    }

    if (m_RenderFinishedSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(VulkanLogicalDevice, m_RenderFinishedSemaphore, nullptr);
        m_RenderFinishedSemaphore = VK_NULL_HANDLE;
    }

    if (m_Fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(VulkanLogicalDevice, m_Fence, nullptr);
        m_Fence = VK_NULL_HANDLE;
    }

    m_SynchronizationObjectsCreated = false;
}

std::optional<std::int32_t> VulkanCommandsManager::DrawFrame() const
{
    if (!m_SynchronizationObjectsCreated)
    {
        return std::nullopt;
    }

    WaitAndResetFences();

    if (m_ImageAvailableSemaphore == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan semaphore: Image Available is invalid.");
    }

    if (m_Fence == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan fence is invalid.");
    }

    std::uint32_t Output = 0U;
    if (VkResult const OperationResult = vkAcquireNextImageKHR(
                VulkanDeviceManager::Get().GetLogicalDevice(),
                VulkanBufferManager::Get().GetSwapChain(),
                g_Timeout,
                m_ImageAvailableSemaphore,
                m_Fence,
                &Output);
        OperationResult != VK_SUCCESS)
    {
        if (OperationResult == VK_ERROR_OUT_OF_DATE_KHR || OperationResult == VK_SUBOPTIMAL_KHR)
        {
            if (OperationResult == VK_ERROR_OUT_OF_DATE_KHR)
            {
                BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Failed to acquire next image: Vulkan swap chain is outdated";
            }
            else if (OperationResult == VK_SUBOPTIMAL_KHR)
            {
                BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Failed to acquire next image: Vulkan swap chain is suboptimal";
                WaitAndResetFences();
            }

            return std::nullopt;
        }
        if (OperationResult != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Failed to acquire Vulkan swap chain image.");
        }
    }

    return static_cast<std::int32_t>(Output);
}

void VulkanCommandsManager::RecordCommandBuffers(std::uint32_t const ImageIndex)
{
    AllocateCommandBuffer();

    constexpr VkCommandBufferBeginInfo CommandBufferBeginInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    RenderCoreHelpers::CheckVulkanResult(vkBeginCommandBuffer(m_CommandBuffer, &CommandBufferBeginInfo));

    VkRenderPass const& RenderPass                     = VulkanPipelineManager::Get().GetRenderPass();
    VkPipeline const& Pipeline                         = VulkanPipelineManager::Get().GetPipeline();
    VkPipelineLayout const& PipelineLayout             = VulkanPipelineManager::Get().GetPipelineLayout();
    std::vector<VkDescriptorSet> const& DescriptorSets = VulkanPipelineManager::Get().GetDescriptorSets();

    std::vector<VkFramebuffer> const& FrameBuffers = VulkanBufferManager::Get().GetFrameBuffers();
    VkBuffer const& VertexBuffer                   = VulkanBufferManager::Get().GetVertexBuffer();
    VkBuffer const& IndexBuffer                    = VulkanBufferManager::Get().GetIndexBuffer();
    std::uint32_t const IndexCount                 = VulkanBufferManager::Get().GetIndicesCount();
    UniformBufferObject const UniformBufferObj     = RenderCoreHelpers::GetUniformBufferObject();

    std::vector<VkDeviceSize> const Offsets = {
            0U};
    VkExtent2D const Extent = VulkanBufferManager::Get().GetSwapChainExtent();

    VkViewport const Viewport {
            .x        = 0.F,
            .y        = 0.F,
            .width    = static_cast<float>(Extent.width),
            .height   = static_cast<float>(Extent.height),
            .minDepth = 0.F,
            .maxDepth = 1.F};

    VkRect2D const Scissor {
            .offset = {
                    0,
                    0},
            .extent = Extent};

    bool ActiveRenderPass = false;
    if (RenderPass != VK_NULL_HANDLE && !FrameBuffers.empty())
    {
        VkRenderPassBeginInfo const RenderPassBeginInfo {
                .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass  = RenderPass,
                .framebuffer = FrameBuffers[ImageIndex],
                .renderArea  = {
                         .offset = {
                                0,
                                0},
                         .extent = Extent},
                .clearValueCount = static_cast<std::uint32_t>(g_ClearValues.size()),
                .pClearValues    = g_ClearValues.data()};

        vkCmdBeginRenderPass(m_CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        ActiveRenderPass = true;
    }

    if (Pipeline != VK_NULL_HANDLE)
    {
        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
    }

    if (!DescriptorSets.empty())
    {
        VkDescriptorSet const& DescriptorSet = DescriptorSets[m_FrameIndex];
        vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0U, 1U, &DescriptorSet, 0U, nullptr);
    }

    vkCmdPushConstants(m_CommandBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0U, sizeof(UniformBufferObject), &UniformBufferObj);

    bool ActiveVertexBinding = false;
    if (VertexBuffer != VK_NULL_HANDLE)
    {
        vkCmdBindVertexBuffers(m_CommandBuffer, 0U, 1U, &VertexBuffer, Offsets.data());
        ActiveVertexBinding = true;
    }

    bool ActiveIndexBinding = false;
    if (IndexBuffer != VK_NULL_HANDLE)
    {
        vkCmdBindIndexBuffer(m_CommandBuffer, IndexBuffer, 0U, VK_INDEX_TYPE_UINT32);
        ActiveIndexBinding = true;
    }

    vkCmdSetViewport(m_CommandBuffer, 0U, 1U, &Viewport);
    vkCmdSetScissor(m_CommandBuffer, 0U, 1U, &Scissor);

    if (ActiveRenderPass && ActiveVertexBinding && ActiveIndexBinding)
    {
        vkCmdDrawIndexed(m_CommandBuffer, IndexCount, 1U, 0U, 0U, 0U);
    }

    if (ActiveRenderPass)
    {
        vkCmdEndRenderPass(m_CommandBuffer);
    }

    RenderCoreHelpers::CheckVulkanResult(vkEndCommandBuffer(m_CommandBuffer));
}

void VulkanCommandsManager::SubmitCommandBuffers()
{
    WaitAndResetFences();

    constexpr std::array<VkPipelineStageFlags, 1U> WaitStages {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo const SubmitInfo {
            .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount   = 1U,
            .pWaitSemaphores      = &m_ImageAvailableSemaphore,
            .pWaitDstStageMask    = WaitStages.data(),
            .commandBufferCount   = 1U,
            .pCommandBuffers      = &m_CommandBuffer,
            .signalSemaphoreCount = 1U,
            .pSignalSemaphores    = &m_RenderFinishedSemaphore};

    VkQueue const& GraphicsQueue = VulkanDeviceManager::Get().GetGraphicsQueue().second;

    RenderCoreHelpers::CheckVulkanResult(vkQueueSubmit(GraphicsQueue, 1U, &SubmitInfo, m_Fence));
    RenderCoreHelpers::CheckVulkanResult(vkQueueWaitIdle(GraphicsQueue));

    vkFreeCommandBuffers(VulkanDeviceManager::Get().GetLogicalDevice(), m_CommandPool, 1U, &m_CommandBuffer);
    m_CommandBuffer = VK_NULL_HANDLE;
}

void VulkanCommandsManager::PresentFrame(std::uint32_t const ImageIndice)
{
    VkPresentInfoKHR const PresentInfo {
            .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1U,
            .pWaitSemaphores    = &m_RenderFinishedSemaphore,
            .swapchainCount     = 1U,
            .pSwapchains        = &VulkanBufferManager::Get().GetSwapChain(),
            .pImageIndices      = &ImageIndice,
            .pResults           = nullptr};

    if (VkResult const OperationResult = vkQueuePresentKHR(VulkanDeviceManager::Get().GetGraphicsQueue().second, &PresentInfo);
        OperationResult != VK_SUCCESS)
    {
        if (OperationResult != VK_ERROR_OUT_OF_DATE_KHR && OperationResult != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Vulkan operation failed with result: " + std::string(ResultToString(OperationResult)));
        }
    }

    m_FrameIndex = (m_FrameIndex + 1U) % g_MaxFramesInFlight;
}

void VulkanCommandsManager::CreateGraphicsCommandPool()
{
    m_CommandPool = CreateCommandPool(VulkanDeviceManager::Get().GetGraphicsQueue().first);
}

void VulkanCommandsManager::AllocateCommandBuffer()
{
    VkDevice const& VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

    if (m_CommandBuffer != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(VulkanLogicalDevice, m_CommandPool, 1U, &m_CommandBuffer);
        m_CommandBuffer = VK_NULL_HANDLE;
    }

    if (m_CommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(VulkanLogicalDevice, m_CommandPool, nullptr);
        m_CommandPool = VK_NULL_HANDLE;
    }

    CreateGraphicsCommandPool();

    VkCommandBufferAllocateInfo const CommandBufferAllocateInfo {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = m_CommandPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1U};

    RenderCoreHelpers::CheckVulkanResult(vkAllocateCommandBuffers(VulkanLogicalDevice, &CommandBufferAllocateInfo, &m_CommandBuffer));
}

void VulkanCommandsManager::WaitAndResetFences() const
{
    if (m_Fence == VK_NULL_HANDLE)
    {
        return;
    }

    VkDevice const& VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

    RenderCoreHelpers::CheckVulkanResult(vkWaitForFences(VulkanLogicalDevice, 1U, &m_Fence, VK_TRUE, g_Timeout));
    RenderCoreHelpers::CheckVulkanResult(vkResetFences(VulkanLogicalDevice, 1U, &m_Fence));
}
