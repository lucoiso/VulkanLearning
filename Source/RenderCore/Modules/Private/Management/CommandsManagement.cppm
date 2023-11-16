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

import RenderCore.EngineCore;
import RenderCore.Management.DeviceManagement;
import RenderCore.Management.BufferManagement;
import RenderCore.Management.PipelineManagement;
import RenderCore.Types.Camera;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import RenderCore.Utils.EnumConverter;
import Timer.ExecutionCounter;

using namespace RenderCore;

VkCommandPool g_CommandPool {};
std::array<VkCommandBuffer, 1U> g_CommandBuffers {};
VkSemaphore g_ImageAvailableSemaphore {};
VkSemaphore g_RenderFinishedSemaphore {};
VkFence g_Fence {};
bool g_SynchronizationObjectsCreated {};

void CreateGraphicsCommandPool()
{
    g_CommandPool = RenderCore::CreateCommandPool(GetGraphicsQueue().first);
}

void AllocateCommandBuffer()
{
    VkDevice const& VulkanLogicalDevice = volkGetLoadedDevice();

    for (VkCommandBuffer& CommandBufferIter: g_CommandBuffers)
    {
        if (CommandBufferIter != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(VulkanLogicalDevice, g_CommandPool, 1U, &CommandBufferIter);
            CommandBufferIter = VK_NULL_HANDLE;
        }
    }

    if (g_CommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(VulkanLogicalDevice, g_CommandPool, nullptr);
        g_CommandPool = VK_NULL_HANDLE;
    }

    CreateGraphicsCommandPool();

    VkCommandBufferAllocateInfo const CommandBufferAllocateInfo {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = g_CommandPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<std::uint32_t>(std::size(g_CommandBuffers))};

    CheckVulkanResult(vkAllocateCommandBuffers(VulkanLogicalDevice, &CommandBufferAllocateInfo, std::data(g_CommandBuffers)));
}

void WaitAndResetFences()
{
    if (g_Fence == VK_NULL_HANDLE)
    {
        return;
    }

    VkDevice const& VulkanLogicalDevice = volkGetLoadedDevice();

    CheckVulkanResult(vkWaitForFences(VulkanLogicalDevice, 1U, &g_Fence, VK_TRUE, g_Timeout));
    CheckVulkanResult(vkResetFences(VulkanLogicalDevice, 1U, &g_Fence));
}

void FreeCommandBuffers()
{
    vkFreeCommandBuffers(volkGetLoadedDevice(), g_CommandPool, static_cast<std::uint32_t>(std::size(g_CommandBuffers)), std::data(g_CommandBuffers));

    for (VkCommandBuffer& CommandBufferIter: g_CommandBuffers)
    {
        CommandBufferIter = VK_NULL_HANDLE;
    }
}

void RenderCore::ReleaseCommandsResources()
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

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
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    if (g_SynchronizationObjectsCreated)
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan synchronization objects";

    constexpr VkSemaphoreCreateInfo SemaphoreCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    constexpr VkFenceCreateInfo FenceCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT};

    VkDevice const& VulkanLogicalDevice = volkGetLoadedDevice();

    CheckVulkanResult(vkCreateSemaphore(VulkanLogicalDevice, &SemaphoreCreateInfo, nullptr, &g_ImageAvailableSemaphore));
    CheckVulkanResult(vkCreateSemaphore(VulkanLogicalDevice, &SemaphoreCreateInfo, nullptr, &g_RenderFinishedSemaphore));
    CheckVulkanResult(vkCreateFence(VulkanLogicalDevice, &FenceCreateInfo, nullptr, &g_Fence));

    g_SynchronizationObjectsCreated = true;
}

void RenderCore::DestroyCommandsSynchronizationObjects()
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    if (!g_SynchronizationObjectsCreated)
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying vulkan synchronization objects";

    VkDevice const& VulkanLogicalDevice = volkGetLoadedDevice();

    vkDeviceWaitIdle(VulkanLogicalDevice);

    FreeCommandBuffers();

    if (g_CommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(VulkanLogicalDevice, g_CommandPool, nullptr);
        g_CommandPool = VK_NULL_HANDLE;
    }

    if (g_ImageAvailableSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(VulkanLogicalDevice, g_ImageAvailableSemaphore, nullptr);
        g_ImageAvailableSemaphore = VK_NULL_HANDLE;
    }

    if (g_RenderFinishedSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(VulkanLogicalDevice, g_RenderFinishedSemaphore, nullptr);
        g_RenderFinishedSemaphore = VK_NULL_HANDLE;
    }

    if (g_Fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(VulkanLogicalDevice, g_Fence, nullptr);
        g_Fence = VK_NULL_HANDLE;
    }

    g_SynchronizationObjectsCreated = false;
}

std::optional<std::int32_t> RenderCore::RequestSwapChainImage()
{
    if (!g_SynchronizationObjectsCreated)
    {
        return std::nullopt;
    }

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
                GetSwapChain(),
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

void RenderCore::RecordCommandBuffers(std::uint32_t const ImageIndex)
{
    AllocateCommandBuffer();

    VkCommandBuffer const& MainCommandBuffer = g_CommandBuffers.at(0U);

    constexpr VkCommandBufferBeginInfo CommandBufferBeginInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    CheckVulkanResult(vkBeginCommandBuffer(MainCommandBuffer, &CommandBufferBeginInfo));

    VkRenderPass const& RenderPass                 = GetRenderPass();
    VkPipeline const& Pipeline                     = GetPipeline();
    VkPipelineLayout const& PipelineLayout         = GetPipelineLayout();
    std::vector<VkFramebuffer> const& FrameBuffers = GetFrameBuffers();

    constexpr std::array<VkDeviceSize, 1U> Offsets {0U};

    VkExtent2D const Extent = GetSwapChainExtent();

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
        for (std::shared_ptr<Object> const& ObjectIter: EngineCore::Get().GetObjects())
        {
            if (!ObjectIter || ObjectIter->IsPendingDestroy() || !GetViewportCamera().CanDrawObject(ObjectIter))
            {
                continue;
            }

            std::uint32_t const ObjectID = ObjectIter->GetID();

            if (VkDescriptorSet const& DescriptorSet = GetDescriptorSet(ObjectID); DescriptorSet != VK_NULL_HANDLE)
            {
                vkCmdBindDescriptorSets(MainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0U, 1U, &DescriptorSet, 0U, nullptr);
            }
            else
            {
                continue;
            }

            VkBuffer const& VertexBuffer   = GetVertexBuffer(ObjectID);
            VkBuffer const& IndexBuffer    = GetIndexBuffer(ObjectID);
            std::uint32_t const IndexCount = GetIndicesCount(ObjectID);

            UpdateUniformBuffers(ObjectID);

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

        if (ImDrawData* const ImGuiDrawData = ImGui::GetDrawData())
        {
            ImGui_ImplVulkan_RenderDrawData(ImGuiDrawData, MainCommandBuffer);
        }

        vkCmdEndRenderPass(MainCommandBuffer);
    }

    CheckVulkanResult(vkEndCommandBuffer(MainCommandBuffer));
}

void RenderCore::SubmitCommandBuffers()
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

    VkQueue const& GraphicsQueue = GetGraphicsQueue().second;

    CheckVulkanResult(vkQueueSubmit(GraphicsQueue, 1U, &SubmitInfo, g_Fence));
    CheckVulkanResult(vkQueueWaitIdle(GraphicsQueue));

    FreeCommandBuffers();
}

void RenderCore::PresentFrame(std::uint32_t const ImageIndice)
{
    VkPresentInfoKHR const PresentInfo {
            .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1U,
            .pWaitSemaphores    = &g_RenderFinishedSemaphore,
            .swapchainCount     = 1U,
            .pSwapchains        = &GetSwapChain(),
            .pImageIndices      = &ImageIndice,
            .pResults           = nullptr};

    if (VkResult const OperationResult = vkQueuePresentKHR(GetGraphicsQueue().second, &PresentInfo);
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

    VkDevice const& VulkanLogicalDevice = volkGetLoadedDevice();

    VkCommandPoolCreateInfo const CommandPoolCreateInfo {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = static_cast<std::uint32_t>(QueueFamilyIndex)};

    CheckVulkanResult(vkCreateCommandPool(VulkanLogicalDevice, &CommandPoolCreateInfo, nullptr, &CommandPool));

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

    CheckVulkanResult(vkAllocateCommandBuffers(VulkanLogicalDevice, &CommandBufferAllocateInfo, &CommandBuffer));
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

    VkDevice const& VulkanLogicalDevice = volkGetLoadedDevice();

    vkFreeCommandBuffers(VulkanLogicalDevice, CommandPool, 1U, &CommandBuffer);
    vkDestroyCommandPool(VulkanLogicalDevice, CommandPool, nullptr);
}