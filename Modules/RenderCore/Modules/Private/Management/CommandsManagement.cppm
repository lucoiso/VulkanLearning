// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <boost/log/trivial.hpp>
#include <volk.h>

module RenderCore.Management.CommandsManagement;

import <limits>;
import <optional>;
import <vector>;
import <array>;
import <cstdint>;
import <optional>;
import <stdexcept>;
import <string>;
import <span>;

import RenderCore.Utils.Constants;
import RenderCore.Management.BufferManagement;
import RenderCore.Management.DeviceManagement;
import RenderCore.Management.PipelineManagement;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Types.Vertex;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.EnumConverter;

using namespace RenderCore;

static VkCommandPool s_CommandPool {};
static std::array<VkCommandBuffer, 1U> s_CommandBuffers {};
static VkSemaphore s_ImageAvailableSemaphore {};
static VkSemaphore s_RenderFinishedSemaphore {};
static VkFence s_Fence {};
static bool s_SynchronizationObjectsCreated {};
static std::vector<struct ObjectAllocation> s_ImGuiFontsAllocation {};

void CreateGraphicsCommandPool()
{
    s_CommandPool = RenderCore::CreateCommandPool(GetGraphicsQueue().first);
}

void AllocateCommandBuffer()
{
    VkDevice const& VulkanLogicalDevice = GetLogicalDevice();

    for (VkCommandBuffer& CommandBufferIter: s_CommandBuffers)
    {
        if (CommandBufferIter != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(VulkanLogicalDevice, s_CommandPool, 1U, &CommandBufferIter);
            CommandBufferIter = VK_NULL_HANDLE;
        }
    }

    if (s_CommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(VulkanLogicalDevice, s_CommandPool, nullptr);
        s_CommandPool = VK_NULL_HANDLE;
    }

    CreateGraphicsCommandPool();

    VkCommandBufferAllocateInfo const CommandBufferAllocateInfo {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = s_CommandPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<std::uint32_t>(s_CommandBuffers.size())};

    CheckVulkanResult(vkAllocateCommandBuffers(VulkanLogicalDevice, &CommandBufferAllocateInfo, s_CommandBuffers.data()));
}

void WaitAndResetFences()
{
    if (s_Fence == VK_NULL_HANDLE)
    {
        return;
    }

    VkDevice const& VulkanLogicalDevice = GetLogicalDevice();

    CheckVulkanResult(vkWaitForFences(VulkanLogicalDevice, 1U, &s_Fence, VK_TRUE, g_Timeout));
    CheckVulkanResult(vkResetFences(VulkanLogicalDevice, 1U, &s_Fence));
}

void ResetImGuiFontsAllocation()
{
    for (ObjectAllocation& ObjectAllocationIter: s_ImGuiFontsAllocation)
    {
        ObjectAllocationIter.DestroyResources();
    }
    s_ImGuiFontsAllocation.clear();
}

void FreeCommandBuffers()
{
    vkFreeCommandBuffers(GetLogicalDevice(), s_CommandPool, static_cast<std::uint32_t>(s_CommandBuffers.size()), s_CommandBuffers.data());

    for (VkCommandBuffer& CommandBufferIter: s_CommandBuffers)
    {
        CommandBufferIter = VK_NULL_HANDLE;
    }
}

void RenderCore::ReleaseCommandsResources()
{
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
    CheckVulkanResult(vkCreateCommandPool(GetLogicalDevice(), &CommandPoolCreateInfo, nullptr, &Output));

    return Output;
}

void RenderCore::CreateCommandsSynchronizationObjects()
{
    if (s_SynchronizationObjectsCreated)
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan synchronization objects";

    constexpr VkSemaphoreCreateInfo SemaphoreCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    constexpr VkFenceCreateInfo FenceCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT};

    VkDevice const& VulkanLogicalDevice = GetLogicalDevice();

    CheckVulkanResult(vkCreateSemaphore(VulkanLogicalDevice, &SemaphoreCreateInfo, nullptr, &s_ImageAvailableSemaphore));
    CheckVulkanResult(vkCreateSemaphore(VulkanLogicalDevice, &SemaphoreCreateInfo, nullptr, &s_RenderFinishedSemaphore));
    CheckVulkanResult(vkCreateFence(VulkanLogicalDevice, &FenceCreateInfo, nullptr, &s_Fence));

    s_SynchronizationObjectsCreated = true;
}

void RenderCore::DestroyCommandsSynchronizationObjects()
{
    if (!s_SynchronizationObjectsCreated)
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying vulkan synchronization objects";

    VkDevice const& VulkanLogicalDevice = GetLogicalDevice();

    vkDeviceWaitIdle(VulkanLogicalDevice);

    FreeCommandBuffers();

    if (s_CommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(VulkanLogicalDevice, s_CommandPool, nullptr);
        s_CommandPool = VK_NULL_HANDLE;
    }

    if (s_ImageAvailableSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(VulkanLogicalDevice, s_ImageAvailableSemaphore, nullptr);
        s_ImageAvailableSemaphore = VK_NULL_HANDLE;
    }

    if (s_RenderFinishedSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(VulkanLogicalDevice, s_RenderFinishedSemaphore, nullptr);
        s_RenderFinishedSemaphore = VK_NULL_HANDLE;
    }

    if (s_Fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(VulkanLogicalDevice, s_Fence, nullptr);
        s_Fence = VK_NULL_HANDLE;
    }

    ResetImGuiFontsAllocation();

    s_SynchronizationObjectsCreated = false;
}

std::optional<std::int32_t> RenderCore::RequestSwapChainImage()
{
    if (!s_SynchronizationObjectsCreated)
    {
        return std::nullopt;
    }

    WaitAndResetFences();

    if (s_ImageAvailableSemaphore == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan semaphore: Image Available is invalid.");
    }

    if (s_Fence == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan fence is invalid.");
    }

    std::uint32_t Output = 0U;
    if (VkResult const OperationResult = vkAcquireNextImageKHR(
                GetLogicalDevice(),
                GetSwapChain(),
                g_Timeout,
                s_ImageAvailableSemaphore,
                s_Fence,
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
        else
        {
            throw std::runtime_error("Failed to acquire Vulkan swap chain image.");
        }
    }

    return static_cast<std::int32_t>(Output);
}

void RenderCore::RecordCommandBuffers(std::uint32_t const ImageIndex)
{
    AllocateCommandBuffer();

    VkCommandBuffer& MainCommandBuffer = s_CommandBuffers.at(0U);

    constexpr VkCommandBufferBeginInfo CommandBufferBeginInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    CheckVulkanResult(vkBeginCommandBuffer(MainCommandBuffer, &CommandBufferBeginInfo));

    VkRenderPass const& RenderPass                     = GetRenderPass();
    VkPipeline const& Pipeline                         = GetPipeline();
    VkPipelineLayout const& PipelineLayout             = GetPipelineLayout();
    std::vector<VkDescriptorSet> const& DescriptorSets = GetDescriptorSets();

    std::vector<VkFramebuffer> const& FrameBuffers = GetFrameBuffers();
    VkBuffer const& VertexBuffer                   = GetVertexBuffer(0U);
    VkBuffer const& IndexBuffer                    = GetIndexBuffer(0U);
    std::uint32_t const IndexCount                 = GetIndicesCount(0U);
    UniformBufferObject const UniformBufferObj     = GetUniformBufferObject();

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

    bool ActiveRenderPass = false;
    if (RenderPass != VK_NULL_HANDLE && !FrameBuffers.empty())
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
                .clearValueCount = static_cast<std::uint32_t>(g_ClearValues.size()),
                .pClearValues    = g_ClearValues.data()};

        vkCmdBeginRenderPass(MainCommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        ActiveRenderPass = true;
    }

    if (Pipeline != VK_NULL_HANDLE)
    {
        vkCmdBindPipeline(MainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
    }

    if (!DescriptorSets.empty())
    {
        std::vector<VkDescriptorSet> ValidDescriptorSets;
        for (VkDescriptorSet const& DescriptorSetIter: DescriptorSets)
        {
            if (DescriptorSetIter != VK_NULL_HANDLE)
            {
                ValidDescriptorSets.push_back(DescriptorSetIter);
            }
        }

        vkCmdBindDescriptorSets(MainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0U, static_cast<std::uint32_t>(ValidDescriptorSets.size()), ValidDescriptorSets.data(), 0U, nullptr);
    }

    vkCmdPushConstants(MainCommandBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0U, sizeof(UniformBufferObj), &UniformBufferObj);

    bool ActiveVertexBinding = false;
    if (VertexBuffer != VK_NULL_HANDLE)
    {
        vkCmdBindVertexBuffers(MainCommandBuffer, 0U, 1U, &VertexBuffer, Offsets.data());
        ActiveVertexBinding = true;
    }

    bool ActiveIndexBinding = false;
    if (IndexBuffer != VK_NULL_HANDLE)
    {
        vkCmdBindIndexBuffer(MainCommandBuffer, IndexBuffer, 0U, VK_INDEX_TYPE_UINT32);
        ActiveIndexBinding = true;
    }

    vkCmdSetViewport(MainCommandBuffer, 0U, 1U, &Viewport);
    vkCmdSetScissor(MainCommandBuffer, 0U, 1U, &Scissor);

    if (ActiveRenderPass && ActiveVertexBinding && ActiveIndexBinding)
    {
        vkCmdDrawIndexed(MainCommandBuffer, IndexCount, 1U, 0U, 0U, 0U);
    }

    if (ActiveRenderPass)
    {
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
            .pWaitSemaphores      = &s_ImageAvailableSemaphore,
            .pWaitDstStageMask    = WaitStages.data(),
            .commandBufferCount   = static_cast<std::uint32_t>(s_CommandBuffers.size()),
            .pCommandBuffers      = s_CommandBuffers.data(),
            .signalSemaphoreCount = 1U,
            .pSignalSemaphores    = &s_RenderFinishedSemaphore};

    VkQueue const& GraphicsQueue = GetGraphicsQueue().second;

    CheckVulkanResult(vkQueueSubmit(GraphicsQueue, 1U, &SubmitInfo, s_Fence));
    CheckVulkanResult(vkQueueWaitIdle(GraphicsQueue));

    FreeCommandBuffers();
}

void RenderCore::PresentFrame(std::uint32_t const ImageIndice)
{
    VkPresentInfoKHR const PresentInfo {
            .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1U,
            .pWaitSemaphores    = &s_RenderFinishedSemaphore,
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