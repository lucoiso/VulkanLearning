// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <boost/log/trivial.hpp>
#include <volk.h>

/* ImGui Headers */
#include <imgui.h>
#include <imgui_impl_vulkan.h>

module RenderCore.Management.CommandsManagement;

import <array>;

import RenderCore.Renderer;
import RenderCore.Management.DeviceManagement;
import RenderCore.Management.BufferManagement;
import RenderCore.Management.PipelineManagement;
import RenderCore.Types.Camera;
import RenderCore.Types.Object;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import RenderCore.Utils.EnumConverter;
import Timer.ExecutionCounter;

using namespace RenderCore;

void CommandsManager::AllocateCommandBuffer(std::uint32_t const QueueFamily)
{
    if (std::empty(m_CommandBuffers))
    {
        m_CommandBuffers.emplace_back();
    }
    else
    {
        vkFreeCommandBuffers(volkGetLoadedDevice(), m_CommandPool, static_cast<std::uint32_t>(std::size(m_CommandBuffers)), std::data(m_CommandBuffers));
        m_CommandBuffers.clear();
    }

    if (m_CommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(volkGetLoadedDevice(), m_CommandPool, nullptr);
        m_CommandPool = VK_NULL_HANDLE;
    }

    m_CommandPool = CreateCommandPool(QueueFamily);

    VkCommandBufferAllocateInfo const CommandBufferAllocateInfo {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = m_CommandPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<std::uint32_t>(std::size(m_CommandBuffers))};

    CheckVulkanResult(vkAllocateCommandBuffers(volkGetLoadedDevice(), &CommandBufferAllocateInfo, std::data(m_CommandBuffers)));
}

void CommandsManager::WaitAndResetFences() const
{
    if (m_Fence == VK_NULL_HANDLE)
    {
        return;
    }

    CheckVulkanResult(vkWaitForFences(volkGetLoadedDevice(), 1U, &m_Fence, VK_TRUE, g_Timeout));
    CheckVulkanResult(vkResetFences(volkGetLoadedDevice(), 1U, &m_Fence));
}

void CommandsManager::FreeCommandBuffers()
{
    if (!std::empty(m_CommandBuffers))
    {
        vkFreeCommandBuffers(volkGetLoadedDevice(), m_CommandPool, static_cast<std::uint32_t>(std::size(m_CommandBuffers)), std::data(m_CommandBuffers));

        for (VkCommandBuffer& CommandBufferIter: m_CommandBuffers)
        {
            CommandBufferIter = VK_NULL_HANDLE;
        }
        m_CommandBuffers.clear();
    }
}

void CommandsManager::ReleaseCommandsResources()
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Releasing vulkan commands resources";

    DestroyCommandsSynchronizationObjects();
}

VkCommandPool CommandsManager::CreateCommandPool(std::uint8_t const FamilyQueueIndex)
{
    VkCommandPoolCreateInfo const CommandPoolCreateInfo {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = static_cast<std::uint32_t>(FamilyQueueIndex)};

    VkCommandPool Output = VK_NULL_HANDLE;
    CheckVulkanResult(vkCreateCommandPool(volkGetLoadedDevice(), &CommandPoolCreateInfo, nullptr, &Output));

    return Output;
}

void CommandsManager::CreateCommandsSynchronizationObjects()
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan synchronization objects";

    constexpr VkSemaphoreCreateInfo SemaphoreCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    constexpr VkFenceCreateInfo FenceCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT};

    CheckVulkanResult(vkCreateSemaphore(volkGetLoadedDevice(), &SemaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphore));
    CheckVulkanResult(vkCreateSemaphore(volkGetLoadedDevice(), &SemaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphore));
    CheckVulkanResult(vkCreateFence(volkGetLoadedDevice(), &FenceCreateInfo, nullptr, &m_Fence));
}

void CommandsManager::DestroyCommandsSynchronizationObjects()
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying vulkan synchronization objects";

    vkDeviceWaitIdle(volkGetLoadedDevice());

    FreeCommandBuffers();

    if (m_CommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(volkGetLoadedDevice(), m_CommandPool, nullptr);
        m_CommandPool = VK_NULL_HANDLE;
    }

    if (m_ImageAvailableSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(volkGetLoadedDevice(), m_ImageAvailableSemaphore, nullptr);
        m_ImageAvailableSemaphore = VK_NULL_HANDLE;
    }

    if (m_RenderFinishedSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(volkGetLoadedDevice(), m_RenderFinishedSemaphore, nullptr);
        m_RenderFinishedSemaphore = VK_NULL_HANDLE;
    }

    if (m_Fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(volkGetLoadedDevice(), m_Fence, nullptr);
        m_Fence = VK_NULL_HANDLE;
    }
}

std::optional<std::int32_t> CommandsManager::RequestSwapChainImage(VkSwapchainKHR const& SwapChain) const
{
    WaitAndResetFences();

    if (m_ImageAvailableSemaphore == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan semaphore: ImageAllocation Available is invalid.");
    }

    if (m_Fence == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan fence is invalid.");
    }

    std::uint32_t Output = 0U;
    if (VkResult const OperationResult = vkAcquireNextImageKHR(
                volkGetLoadedDevice(),
                SwapChain,
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
            else
            {
                BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Failed to acquire next image: Vulkan swap chain is suboptimal";
                WaitAndResetFences();
            }

            return std::nullopt;
        }

        throw std::runtime_error("Failed to acquire Vulkan swap chain image.");
    }

    return static_cast<std::int32_t>(Output);
}

void CommandsManager::RecordCommandBuffers(std::uint32_t const QueueFamily, std::uint32_t const ImageIndex, BufferManager& BufferManager, PipelineManager& PipelineManager, std::vector<std::shared_ptr<Object>> const& Objects)
{
    AllocateCommandBuffer(QueueFamily);
    VkCommandBuffer const& MainCommandBuffer = m_CommandBuffers.back();

    constexpr VkCommandBufferBeginInfo CommandBufferBeginInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    CheckVulkanResult(vkBeginCommandBuffer(MainCommandBuffer, &CommandBufferBeginInfo));

    VkRenderPass const& RenderPass                 = PipelineManager.GetRenderPass();
    VkPipeline const& Pipeline                     = PipelineManager.GetPipeline();
    VkPipelineLayout const& PipelineLayout         = PipelineManager.GetPipelineLayout();
    std::vector<VkFramebuffer> const& FrameBuffers = BufferManager.GetFrameBuffers();

    constexpr std::array<VkDeviceSize, 1U> Offsets {0U};

    VkExtent2D const Extent = BufferManager.GetSwapChainExtent();

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

    vkCmdSetViewport(MainCommandBuffer, 0U, 1U, &Viewport);
    vkCmdSetScissor(MainCommandBuffer, 0U, 1U, &Scissor);

    bool ActiveRenderPass = false;
    if (RenderPass != VK_NULL_HANDLE && !std::empty(FrameBuffers))
    {
        VkRenderPassBeginInfo const RenderPassBeginInfo {
                .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass  = RenderPass,
                .framebuffer = FrameBuffers.at(ImageIndex),
                .renderArea  = {
                         .offset = {
                                0,
                                0},
                         .extent = Extent},
                .clearValueCount = static_cast<std::uint32_t>(std::size(g_ClearValues)),
                .pClearValues    = std::data(g_ClearValues)};

        vkCmdBeginRenderPass(MainCommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        ActiveRenderPass = true;
    }

    if (Pipeline != VK_NULL_HANDLE)
    {
        vkCmdBindPipeline(MainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
    }

    if (ActiveRenderPass)
    {
        for (std::shared_ptr<Object> const& ObjectIter: Objects)
        {
            if (!ObjectIter || ObjectIter->IsPendingDestroy() || !GetViewportCamera().CanDrawObject(ObjectIter, BufferManager.GetSwapChainExtent()))
            {
                continue;
            }

            std::uint32_t const ObjectID = ObjectIter->GetID();

            if (VkDescriptorSet const& DescriptorSet = PipelineManager.GetDescriptorSet(ObjectID); DescriptorSet != VK_NULL_HANDLE)
            {
                vkCmdBindDescriptorSets(MainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0U, 1U, &DescriptorSet, 0U, nullptr);
            }
            else
            {
                continue;
            }

            VkBuffer const& VertexBuffer   = BufferManager.GetVertexBuffer(ObjectID);
            VkBuffer const& IndexBuffer    = BufferManager.GetIndexBuffer(ObjectID);
            std::uint32_t const IndexCount = BufferManager.GetIndicesCount(ObjectID);

            BufferManager.UpdateUniformBuffers(ObjectIter, BufferManager.GetSwapChainExtent());

            bool ActiveVertexBinding {false};
            if (VertexBuffer != VK_NULL_HANDLE)
            {
                vkCmdBindVertexBuffers(MainCommandBuffer, 0U, 1U, &VertexBuffer, std::data(Offsets));
                ActiveVertexBinding = true;
            }

            bool ActiveIndexBinding {false};
            if (IndexBuffer != VK_NULL_HANDLE)
            {
                vkCmdBindIndexBuffer(MainCommandBuffer, IndexBuffer, 0U, VK_INDEX_TYPE_UINT32);
                ActiveIndexBinding = true;
            }

            if (ActiveVertexBinding && ActiveIndexBinding)
            {
                vkCmdDrawIndexed(MainCommandBuffer, IndexCount, 1U, 0U, 0U, 0U);
            }
        }

        if (PipelineManager.GetIsBoundToImGui())
        {
            if (ImDrawData* const ImGuiDrawData = ImGui::GetDrawData())
            {
                ImGui_ImplVulkan_RenderDrawData(ImGuiDrawData, MainCommandBuffer);
            }
        }

        vkCmdEndRenderPass(MainCommandBuffer);
    }

    CheckVulkanResult(vkEndCommandBuffer(MainCommandBuffer));
}

void CommandsManager::SubmitCommandBuffers(VkQueue const& GraphicsQueue)
{
    WaitAndResetFences();

    constexpr std::array<VkPipelineStageFlags, 1U> WaitStages {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo const SubmitInfo {
            .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount   = 1U,
            .pWaitSemaphores      = &m_ImageAvailableSemaphore,
            .pWaitDstStageMask    = std::data(WaitStages),
            .commandBufferCount   = static_cast<std::uint32_t>(std::size(m_CommandBuffers)),
            .pCommandBuffers      = std::data(m_CommandBuffers),
            .signalSemaphoreCount = 1U,
            .pSignalSemaphores    = &m_RenderFinishedSemaphore};

    CheckVulkanResult(vkQueueSubmit(GraphicsQueue, 1U, &SubmitInfo, m_Fence));
    CheckVulkanResult(vkQueueWaitIdle(GraphicsQueue));

    FreeCommandBuffers();
}

void CommandsManager::PresentFrame(VkQueue const& Queue, std::uint32_t const ImageIndice, VkSwapchainKHR const& SwapChain)
{
    VkPresentInfoKHR const PresentInfo {
            .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1U,
            .pWaitSemaphores    = &m_RenderFinishedSemaphore,
            .swapchainCount     = 1U,
            .pSwapchains        = &SwapChain,
            .pImageIndices      = &ImageIndice,
            .pResults           = nullptr};

    if (VkResult const OperationResult = vkQueuePresentKHR(Queue, &PresentInfo);
        OperationResult != VK_SUCCESS)
    {
        if (OperationResult != VK_ERROR_OUT_OF_DATE_KHR && OperationResult != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Vulkan operation failed with result: " + std::string(ResultToString(OperationResult)));
        }
    }
}

void RenderCore::InitializeSingleCommandQueue(VkCommandPool& CommandPool, VkCommandBuffer& CommandBuffer, std::uint8_t const QueueFamilyIndex)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    VkCommandPoolCreateInfo const CommandPoolCreateInfo {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = static_cast<std::uint32_t>(QueueFamilyIndex)};

    CheckVulkanResult(vkCreateCommandPool(volkGetLoadedDevice(), &CommandPoolCreateInfo, nullptr, &CommandPool));

    constexpr VkCommandBufferBeginInfo CommandBufferBeginInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    VkCommandBufferAllocateInfo const CommandBufferAllocateInfo {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = CommandPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1U,
    };

    CheckVulkanResult(vkAllocateCommandBuffers(volkGetLoadedDevice(), &CommandBufferAllocateInfo, &CommandBuffer));
    CheckVulkanResult(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));
}

void RenderCore::FinishSingleCommandQueue(VkQueue const& Queue, VkCommandPool const& CommandPool, VkCommandBuffer const& CommandBuffer)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    if (CommandPool == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan command pool is invalid.");
    }

    if (CommandBuffer == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan command buffer is invalid.");
    }

    CheckVulkanResult(vkEndCommandBuffer(CommandBuffer));

    VkSubmitInfo const SubmitInfo {
            .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1U,
            .pCommandBuffers    = &CommandBuffer,
    };

    CheckVulkanResult(vkQueueSubmit(Queue, 1U, &SubmitInfo, VK_NULL_HANDLE));
    CheckVulkanResult(vkQueueWaitIdle(Queue));

    vkFreeCommandBuffers(volkGetLoadedDevice(), CommandPool, 1U, &CommandBuffer);
    vkDestroyCommandPool(volkGetLoadedDevice(), CommandPool, nullptr);
}