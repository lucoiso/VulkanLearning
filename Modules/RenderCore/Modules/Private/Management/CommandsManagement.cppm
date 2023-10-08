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
            .commandBufferCount = static_cast<std::uint32_t>(g_CommandBuffers.size())};

    CheckVulkanResult(vkAllocateCommandBuffers(VulkanLogicalDevice, &CommandBufferAllocateInfo, g_CommandBuffers.data()));
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
    vkFreeCommandBuffers(volkGetLoadedDevice(), g_CommandPool, static_cast<std::uint32_t>(g_CommandBuffers.size()), g_CommandBuffers.data());

    for (VkCommandBuffer& CommandBufferIter: g_CommandBuffers)
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
    CheckVulkanResult(vkCreateCommandPool(volkGetLoadedDevice(), &CommandPoolCreateInfo, nullptr, &Output));

    return Output;
}

void RenderCore::CreateCommandsSynchronizationObjects()
{
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

    VkCommandBuffer& MainCommandBuffer = g_CommandBuffers.at(0U);

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
            .pWaitDstStageMask    = WaitStages.data(),
            .commandBufferCount   = static_cast<std::uint32_t>(g_CommandBuffers.size()),
            .pCommandBuffers      = g_CommandBuffers.data(),
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