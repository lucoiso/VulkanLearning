// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <array>
#include <boost/log/trivial.hpp>
#include <optional>
#include <volk.h>

/* ImGui Headers */
#include <imgui.h>
#include <imgui_impl_vulkan.h>

module RenderCore.Management.CommandsManagement;

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

VkCommandPool g_CommandPool {};
std::vector<VkCommandBuffer> g_CommandBuffers {};
VkSemaphore g_ImageAvailableSemaphore {};
VkSemaphore g_RenderFinishedSemaphore {};
VkFence g_Fence {};

void AllocateCommandBuffer(std::uint32_t const QueueFamily)
{
    if (std::empty(g_CommandBuffers))
    {
        g_CommandBuffers.emplace_back();
    }
    else
    {
        vkFreeCommandBuffers(volkGetLoadedDevice(), g_CommandPool, static_cast<std::uint32_t>(std::size(g_CommandBuffers)), std::data(g_CommandBuffers));
        g_CommandBuffers.clear();
    }

    if (g_CommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(volkGetLoadedDevice(), g_CommandPool, nullptr);
        g_CommandPool = VK_NULL_HANDLE;
    }

    g_CommandPool = CreateCommandPool(QueueFamily);

    VkCommandBufferAllocateInfo const CommandBufferAllocateInfo {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = g_CommandPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<std::uint32_t>(std::size(g_CommandBuffers))};

    CheckVulkanResult(vkAllocateCommandBuffers(volkGetLoadedDevice(), &CommandBufferAllocateInfo, std::data(g_CommandBuffers)));
}

void WaitAndResetFences()
{
    if (g_Fence == VK_NULL_HANDLE)
    {
        return;
    }

    CheckVulkanResult(vkWaitForFences(volkGetLoadedDevice(), 1U, &g_Fence, VK_TRUE, g_Timeout));
    CheckVulkanResult(vkResetFences(volkGetLoadedDevice(), 1U, &g_Fence));
}

void FreeCommandBuffers()
{
    if (!std::empty(g_CommandBuffers))
    {
        vkFreeCommandBuffers(volkGetLoadedDevice(), g_CommandPool, static_cast<std::uint32_t>(std::size(g_CommandBuffers)), std::data(g_CommandBuffers));

        for (VkCommandBuffer& CommandBufferIter: g_CommandBuffers)
        {
            CommandBufferIter = VK_NULL_HANDLE;
        }
        g_CommandBuffers.clear();
    }
}

void RenderCore::ReleaseCommandsResources()
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Releasing vulkan commands resources";

    DestroyCommandsSynchronizationObjects();
}

VkCommandPool RenderCore::CreateCommandPool(std::uint8_t const FamilyQueueIndex)
{
    VkCommandPoolCreateInfo const CommandPoolCreateInfo {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = static_cast<std::uint32_t>(FamilyQueueIndex)};

    VkCommandPool Output = VK_NULL_HANDLE;
    CheckVulkanResult(vkCreateCommandPool(volkGetLoadedDevice(), &CommandPoolCreateInfo, nullptr, &Output));

    return Output;
}

void RenderCore::CreateCommandsSynchronizationObjects()
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan synchronization objects";

    constexpr VkSemaphoreCreateInfo SemaphoreCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    constexpr VkFenceCreateInfo FenceCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT};

    CheckVulkanResult(vkCreateSemaphore(volkGetLoadedDevice(), &SemaphoreCreateInfo, nullptr, &g_ImageAvailableSemaphore));
    CheckVulkanResult(vkCreateSemaphore(volkGetLoadedDevice(), &SemaphoreCreateInfo, nullptr, &g_RenderFinishedSemaphore));
    CheckVulkanResult(vkCreateFence(volkGetLoadedDevice(), &FenceCreateInfo, nullptr, &g_Fence));
}

void RenderCore::DestroyCommandsSynchronizationObjects()
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying vulkan synchronization objects";

    vkDeviceWaitIdle(volkGetLoadedDevice());

    FreeCommandBuffers();

    if (g_CommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(volkGetLoadedDevice(), g_CommandPool, nullptr);
        g_CommandPool = VK_NULL_HANDLE;
    }

    if (g_ImageAvailableSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(volkGetLoadedDevice(), g_ImageAvailableSemaphore, nullptr);
        g_ImageAvailableSemaphore = VK_NULL_HANDLE;
    }

    if (g_RenderFinishedSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(volkGetLoadedDevice(), g_RenderFinishedSemaphore, nullptr);
        g_RenderFinishedSemaphore = VK_NULL_HANDLE;
    }

    if (g_Fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(volkGetLoadedDevice(), g_Fence, nullptr);
        g_Fence = VK_NULL_HANDLE;
    }
}

std::optional<std::int32_t> RenderCore::RequestSwapChainImage(VkSwapchainKHR const& SwapChain)
{
    WaitAndResetFences();

    if (g_ImageAvailableSemaphore == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan semaphore: ImageAllocation Available is invalid.");
    }

    if (g_Fence == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan fence is invalid.");
    }

    std::uint32_t Output = 0U;
    if (VkResult const OperationResult = vkAcquireNextImageKHR(
                volkGetLoadedDevice(),
                SwapChain,
                g_Timeout,
                g_ImageAvailableSemaphore,
                g_Fence,
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

void RenderCore::RecordCommandBuffers(std::uint32_t const QueueFamily, std::uint32_t const ImageIndex, Camera const& Camera, BufferManager const& BufferManager, PipelineManager const& PipelineManager, std::vector<std::shared_ptr<Object>> const& Objects, ViewSize const& ViewportSize)
{
    AllocateCommandBuffer(QueueFamily);
    VkCommandBuffer const& MainCommandBuffer = g_CommandBuffers.back();

    constexpr VkCommandBufferBeginInfo CommandBufferBeginInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    CheckVulkanResult(vkBeginCommandBuffer(MainCommandBuffer, &CommandBufferBeginInfo));

    VkRenderPass const& RenderPass                 = PipelineManager.GetRenderPass();
    VkPipeline const& Pipeline                     = PipelineManager.GetPipeline();
    VkPipelineLayout const& PipelineLayout         = PipelineManager.GetPipelineLayout();
    std::vector<VkFramebuffer> const& FrameBuffers = BufferManager.GetFrameBuffers();

    constexpr std::array<VkDeviceSize, 1U> Offsets {0U};

    VkViewport const Viewport {
            .x        = ViewportSize.X,
            .y        = ViewportSize.Y,
            .width    = ViewportSize.W,
            .height   = ViewportSize.H,
            .minDepth = 0.F,
            .maxDepth = 1.F};

    VkRect2D const Scissor {
            .offset = {
                    .x = static_cast<std::int32_t>(ViewportSize.X),
                    .y = static_cast<std::int32_t>(ViewportSize.Y)},
            .extent = {
                    .width  = static_cast<std::uint32_t>(ViewportSize.W),
                    .height = static_cast<std::uint32_t>(ViewportSize.H),
            }};

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
                         .extent = BufferManager.GetSwapChainExtent()},
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
            if (!ObjectIter || ObjectIter->IsPendingDestroy() || !Camera.CanDrawObject(ObjectIter, BufferManager.GetSwapChainExtent()))
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

            BufferManager.UpdateUniformBuffers(ObjectIter, Camera, BufferManager.GetSwapChainExtent());

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

void RenderCore::SubmitCommandBuffers(VkQueue const& GraphicsQueue)
{
    WaitAndResetFences();

    constexpr std::array<VkPipelineStageFlags, 1U> WaitStages {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo const SubmitInfo {
            .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount   = 1U,
            .pWaitSemaphores      = &g_ImageAvailableSemaphore,
            .pWaitDstStageMask    = std::data(WaitStages),
            .commandBufferCount   = static_cast<std::uint32_t>(std::size(g_CommandBuffers)),
            .pCommandBuffers      = std::data(g_CommandBuffers),
            .signalSemaphoreCount = 1U,
            .pSignalSemaphores    = &g_RenderFinishedSemaphore};

    CheckVulkanResult(vkQueueSubmit(GraphicsQueue, 1U, &SubmitInfo, g_Fence));
    CheckVulkanResult(vkQueueWaitIdle(GraphicsQueue));

    FreeCommandBuffers();
}

void RenderCore::PresentFrame(VkQueue const& Queue, std::uint32_t const ImageIndice, VkSwapchainKHR const& SwapChain)
{
    VkPresentInfoKHR const PresentInfo {
            .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1U,
            .pWaitSemaphores    = &g_RenderFinishedSemaphore,
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

void RenderCore::InitializeSingleCommandQueue(VkCommandPool& CommandPool, std::vector<VkCommandBuffer>& CommandBuffers, std::uint8_t const QueueFamilyIndex)
{
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
            .commandBufferCount = static_cast<std::uint32_t>(std::size(CommandBuffers)),
    };

    CheckVulkanResult(vkAllocateCommandBuffers(volkGetLoadedDevice(), &CommandBufferAllocateInfo, std::data(CommandBuffers)));
    for (VkCommandBuffer const& CommandBufferIter: CommandBuffers)
    {
        CheckVulkanResult(vkBeginCommandBuffer(CommandBufferIter, &CommandBufferBeginInfo));
    }
}

void RenderCore::FinishSingleCommandQueue(VkQueue const& Queue, VkCommandPool const& CommandPool, std::vector<VkCommandBuffer>& CommandBuffers)
{
    if (CommandPool == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan command pool is invalid.");
    }

    std::erase_if(CommandBuffers, [](VkCommandBuffer const& CommandBufferIter) {
        return CommandBufferIter == VK_NULL_HANDLE;
    });

    if (std::empty(CommandBuffers))
    {
        throw std::runtime_error("Vulkan command buffer is invalid.");
    }

    for (VkCommandBuffer const& CommandBufferIter: CommandBuffers)
    {
        CheckVulkanResult(vkEndCommandBuffer(CommandBufferIter));
    }

    VkSubmitInfo const SubmitInfo {
            .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = static_cast<std::uint32_t>(std::size(CommandBuffers)),
            .pCommandBuffers    = std::data(CommandBuffers),
    };

    CheckVulkanResult(vkQueueSubmit(Queue, 1U, &SubmitInfo, VK_NULL_HANDLE));
    CheckVulkanResult(vkQueueWaitIdle(Queue));

    vkFreeCommandBuffers(volkGetLoadedDevice(), CommandPool, static_cast<std::uint32_t>(std::size(CommandBuffers)), std::data(CommandBuffers));
    vkDestroyCommandPool(volkGetLoadedDevice(), CommandPool, nullptr);
}