// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <boost/log/trivial.hpp>
#include <imgui.h>
#include <volk.h>

module RenderCore.Managers.CommandsManager;

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
import RenderCore.Managers.BufferManager;
import RenderCore.Managers.DeviceManager;
import RenderCore.Managers.PipelineManager;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Types.Vertex;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.EnumConverter;

using namespace RenderCore;

CommandsManager::CommandsManager()
    : m_CommandPool(VK_NULL_HANDLE),
      m_CommandBuffer(VK_NULL_HANDLE),
      m_ImageAvailableSemaphore(VK_NULL_HANDLE),
      m_RenderFinishedSemaphore(VK_NULL_HANDLE),
      m_Fence(VK_NULL_HANDLE),
      m_SynchronizationObjectsCreated(false),
      m_FrameIndex(0U)
{
}

CommandsManager::~CommandsManager()
{
    try
    {
        Shutdown();
    }
    catch (...)
    {
    }
}

CommandsManager& CommandsManager::Get()
{
    static CommandsManager Instance {};
    return Instance;
}

void CommandsManager::Shutdown()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down Vulkan commands manager";

    WaitAndResetFences();

    VkDevice const& VulkanLogicalDevice = DeviceManager::Get().GetLogicalDevice();

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

VkCommandPool CommandsManager::CreateCommandPool(std::uint8_t const FamilyQueueIndex)
{
    VkCommandPoolCreateInfo const CommandPoolCreateInfo {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = static_cast<std::uint32_t>(FamilyQueueIndex)};

    VkCommandPool Output = VK_NULL_HANDLE;
    Helpers::CheckVulkanResult(vkCreateCommandPool(DeviceManager::Get().GetLogicalDevice(), &CommandPoolCreateInfo, nullptr, &Output));

    return Output;
}

void CommandsManager::CreateSynchronizationObjects()
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

    VkDevice const& VulkanLogicalDevice = DeviceManager::Get().GetLogicalDevice();

    Helpers::CheckVulkanResult(vkCreateSemaphore(VulkanLogicalDevice, &SemaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphore));
    Helpers::CheckVulkanResult(vkCreateSemaphore(VulkanLogicalDevice, &SemaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphore));
    Helpers::CheckVulkanResult(vkCreateFence(VulkanLogicalDevice, &FenceCreateInfo, nullptr, &m_Fence));

    m_SynchronizationObjectsCreated = true;
}

void CommandsManager::DestroySynchronizationObjects()
{
    if (!m_SynchronizationObjectsCreated)
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying Vulkan synchronization objects";

    VkDevice const& VulkanLogicalDevice = DeviceManager::Get().GetLogicalDevice();

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

    ResetImGuiFontsAllocation();

    m_SynchronizationObjectsCreated = false;
}

std::optional<std::int32_t> CommandsManager::DrawFrame() const
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
                DeviceManager::Get().GetLogicalDevice(),
                BufferManager::Get().GetSwapChain(),
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
        else
        {
            throw std::runtime_error("Failed to acquire Vulkan swap chain image.");
        }
    }

    return static_cast<std::int32_t>(Output);
}

void CommandsManager::RecordCommandBuffers(std::uint32_t const ImageIndex)
{
    AllocateCommandBuffer();

    constexpr VkCommandBufferBeginInfo CommandBufferBeginInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    Helpers::CheckVulkanResult(vkBeginCommandBuffer(m_CommandBuffer, &CommandBufferBeginInfo));

    VkRenderPass const& RenderPass                     = PipelineManager::Get().GetRenderPass();
    VkPipeline const& Pipeline                         = PipelineManager::Get().GetPipeline();
    VkPipelineLayout const& PipelineLayout             = PipelineManager::Get().GetPipelineLayout();
    std::vector<VkDescriptorSet> const& DescriptorSets = PipelineManager::Get().GetDescriptorSets();

    std::vector<VkFramebuffer> const& FrameBuffers = BufferManager::Get().GetFrameBuffers();
    VkBuffer const& VertexBuffer                   = BufferManager::Get().GetVertexBuffer();
    VkBuffer const& IndexBuffer                    = BufferManager::Get().GetIndexBuffer();
    std::uint32_t const IndexCount                 = BufferManager::Get().GetIndicesCount();
    UniformBufferObject const UniformBufferObj     = Helpers::GetUniformBufferObject();

    std::vector<VkDeviceSize> const Offsets = {
            0U};

    VkExtent2D const Extent = BufferManager::Get().GetSwapChainExtent();

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
        if (ImDrawData const* const ImGuiDrawData = ImGui::GetDrawData())
        {
            ResetImGuiFontsAllocation();

            std::span const ImGuiCmdListSpan(ImGuiDrawData->CmdLists.Data, ImGuiDrawData->CmdListsCount);

            for (std::vector const ImGuiCmdList(ImGuiCmdListSpan.begin(), ImGuiCmdListSpan.end());
                 ImDrawList const* const ImGuiCmdListIter: ImGuiCmdList)
            {
                ObjectAllocation& NewAllocation = m_ImGuiFontsAllocation.emplace_back();

                // Bind Vertex Buffers
                {
                    VkDeviceSize const AllocationSize = ImGuiCmdListIter->VtxBuffer.Size * sizeof(ImDrawVert);

                    std::vector<Vertex> Vertices;
                    Vertices.reserve(ImGuiCmdListIter->VtxBuffer.Size);

                    for (ImDrawVert const& ImGuiVertexIter: ImGuiCmdListIter->VtxBuffer)
                    {
                        Vertices.push_back(
                                Vertex {
                                        .Position          = {ImGuiVertexIter.pos.x, ImGuiVertexIter.pos.y, 0.F},
                                        .Color             = {static_cast<float>(ImGuiVertexIter.col), static_cast<float>(ImGuiVertexIter.col), static_cast<float>(ImGuiVertexIter.col)},
                                        .TextureCoordinate = {ImGuiVertexIter.uv.x, ImGuiVertexIter.uv.y}});
                    }

                    BufferManager::Get().CreateVertexBuffer(NewAllocation, AllocationSize, Vertices);
                    vkCmdBindVertexBuffers(m_CommandBuffer, 0U, 1U, &NewAllocation.VertexBuffer.Buffer, Offsets.data());
                }

                // Bind Index Buffers
                {
                    VkDeviceSize const AllocationSize = ImGuiCmdListIter->IdxBuffer.Size * sizeof(ImDrawIdx);

                    std::vector<std::uint32_t> Indices;
                    Indices.reserve(ImGuiCmdListIter->IdxBuffer.Size);

                    for (ImDrawIdx const& ImGuiIndexIter: ImGuiCmdListIter->IdxBuffer)
                    {
                        Indices.push_back(ImGuiIndexIter);
                    }

                    BufferManager::Get().CreateIndexBuffer(NewAllocation, AllocationSize, Indices);
                    vkCmdBindIndexBuffer(m_CommandBuffer, NewAllocation.IndexBuffer.Buffer, 0U, VK_INDEX_TYPE_UINT32);
                }

                std::span const ImGuiCmdBufferSpan(ImGuiCmdListIter->CmdBuffer.Data, ImGuiCmdListIter->CmdBuffer.Size);
                for (std::vector const ImGuiCmdBuffers(ImGuiCmdBufferSpan.begin(), ImGuiCmdBufferSpan.end());
                     ImDrawCmd const ImGuiCmdBufferIter: ImGuiCmdBuffers)
                {
                    if (ImGuiCmdBufferIter.UserCallback != nullptr)
                    {
                        ImGuiCmdBufferIter.UserCallback(ImGuiCmdListIter, &ImGuiCmdBufferIter);
                    }
                    else
                    {
                        VkRect2D const ImGuiScissor {
                                .offset = {
                                        static_cast<std::int32_t>(ImGuiCmdBufferIter.ClipRect.x),
                                        static_cast<std::int32_t>(ImGuiCmdBufferIter.ClipRect.y)},
                                .extent = {static_cast<std::uint32_t>(ImGuiCmdBufferIter.ClipRect.z - ImGuiCmdBufferIter.ClipRect.x), static_cast<std::uint32_t>(ImGuiCmdBufferIter.ClipRect.w - ImGuiCmdBufferIter.ClipRect.y)}};

                        vkCmdSetScissor(m_CommandBuffer, 0U, 1U, &ImGuiScissor);

                        vkCmdDrawIndexed(
                                m_CommandBuffer,
                                static_cast<std::uint32_t>(ImGuiCmdBufferIter.ElemCount),
                                1U,
                                static_cast<std::uint32_t>(ImGuiCmdBufferIter.IdxOffset),
                                static_cast<std::int32_t>(ImGuiCmdBufferIter.VtxOffset),
                                0U);
                    }
                }
            }
        }
    }

    if (ActiveRenderPass)
    {
        vkCmdEndRenderPass(m_CommandBuffer);
    }

    Helpers::CheckVulkanResult(vkEndCommandBuffer(m_CommandBuffer));
}

void CommandsManager::SubmitCommandBuffers()
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

    VkQueue const& GraphicsQueue = DeviceManager::Get().GetGraphicsQueue().second;

    Helpers::CheckVulkanResult(vkQueueSubmit(GraphicsQueue, 1U, &SubmitInfo, m_Fence));
    Helpers::CheckVulkanResult(vkQueueWaitIdle(GraphicsQueue));

    vkFreeCommandBuffers(DeviceManager::Get().GetLogicalDevice(), m_CommandPool, 1U, &m_CommandBuffer);
    m_CommandBuffer = VK_NULL_HANDLE;
}

void CommandsManager::PresentFrame(std::uint32_t const ImageIndice)
{
    VkPresentInfoKHR const PresentInfo {
            .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1U,
            .pWaitSemaphores    = &m_RenderFinishedSemaphore,
            .swapchainCount     = 1U,
            .pSwapchains        = &BufferManager::Get().GetSwapChain(),
            .pImageIndices      = &ImageIndice,
            .pResults           = nullptr};

    if (VkResult const OperationResult = vkQueuePresentKHR(DeviceManager::Get().GetGraphicsQueue().second, &PresentInfo);
        OperationResult != VK_SUCCESS)
    {
        if (OperationResult != VK_ERROR_OUT_OF_DATE_KHR && OperationResult != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Vulkan operation failed with result: " + std::string(ResultToString(OperationResult)));
        }
    }

    m_FrameIndex = (m_FrameIndex + 1U) % g_MaxFramesInFlight;
}

void CommandsManager::CreateGraphicsCommandPool()
{
    m_CommandPool = CreateCommandPool(DeviceManager::Get().GetGraphicsQueue().first);
}

void CommandsManager::AllocateCommandBuffer()
{
    VkDevice const& VulkanLogicalDevice = DeviceManager::Get().GetLogicalDevice();

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

    Helpers::CheckVulkanResult(vkAllocateCommandBuffers(VulkanLogicalDevice, &CommandBufferAllocateInfo, &m_CommandBuffer));
}

void CommandsManager::WaitAndResetFences() const
{
    if (m_Fence == VK_NULL_HANDLE)
    {
        return;
    }

    VkDevice const& VulkanLogicalDevice = DeviceManager::Get().GetLogicalDevice();

    Helpers::CheckVulkanResult(vkWaitForFences(VulkanLogicalDevice, 1U, &m_Fence, VK_TRUE, g_Timeout));
    Helpers::CheckVulkanResult(vkResetFences(VulkanLogicalDevice, 1U, &m_Fence));
}

void CommandsManager::ResetImGuiFontsAllocation()
{
    for (ObjectAllocation& ObjectAllocationIter: m_ImGuiFontsAllocation)
    {
        ObjectAllocationIter.DestroyResources();
    }
    m_ImGuiFontsAllocation.clear();
}