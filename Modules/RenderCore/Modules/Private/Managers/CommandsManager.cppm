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
      m_CommandBuffers({VK_NULL_HANDLE}),
      m_ImageAvailableSemaphore(VK_NULL_HANDLE),
      m_RenderFinishedSemaphore(VK_NULL_HANDLE),
      m_Fence(VK_NULL_HANDLE),
      m_SynchronizationObjectsCreated(false)
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

    FreeCommandBuffers();

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

    VkCommandBuffer& MainCommandBuffer = m_CommandBuffers.at(0U);

    constexpr VkCommandBufferBeginInfo CommandBufferBeginInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    Helpers::CheckVulkanResult(vkBeginCommandBuffer(MainCommandBuffer, &CommandBufferBeginInfo));

    VkRenderPass const& RenderPass                     = PipelineManager::Get().GetRenderPass();
    VkPipeline const& Pipeline                         = PipelineManager::Get().GetPipeline();
    VkPipelineLayout const& PipelineLayout             = PipelineManager::Get().GetPipelineLayout();
    std::vector<VkDescriptorSet> const& DescriptorSets = PipelineManager::Get().GetDescriptorSets();

    std::vector<VkFramebuffer> const& FrameBuffers = BufferManager::Get().GetFrameBuffers();
    VkBuffer const& VertexBuffer                   = BufferManager::Get().GetVertexBuffer();
    VkBuffer const& IndexBuffer                    = BufferManager::Get().GetIndexBuffer();
    std::uint32_t const IndexCount                 = BufferManager::Get().GetIndicesCount();
    UniformBufferObject const UniformBufferObj     = Helpers::GetUniformBufferObject();

    constexpr std::array<VkDeviceSize, 1U> Offsets {0U};

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

    std::uint32_t PushConstantOffset        = 0U;
    constexpr std::size_t UniformBufferSize = sizeof(UniformBufferObject);
    vkCmdPushConstants(MainCommandBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, PushConstantOffset, UniformBufferSize, &UniformBufferObj);
    PushConstantOffset += static_cast<std::uint32_t>(UniformBufferSize);

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
        if (ImDrawData const* const ImGuiDrawData = ImGui::GetDrawData())
        {
            ResetImGuiFontsAllocation();

            ImVec2 const DisplaySize     = ImGuiDrawData->DisplaySize;
            ImVec2 const DisplayPosition = ImGuiDrawData->DisplayPos;
            ImVec2 const BufferScale     = ImGuiDrawData->FramebufferScale;

            auto const WidthScale  = static_cast<std::int32_t>(DisplaySize.x * ImGuiDrawData->FramebufferScale.x);
            auto const HeightScale = static_cast<std::int32_t>(DisplaySize.y * ImGuiDrawData->FramebufferScale.y);// Setup viewport:

            VkViewport const ImGuiViewport {
                    .x        = 0U,
                    .y        = 0U,
                    .width    = static_cast<float>(WidthScale),
                    .height   = static_cast<float>(HeightScale),
                    .minDepth = 0.F,
                    .maxDepth = 1.F};

            vkCmdSetViewport(MainCommandBuffer, 0U, 1U, &ImGuiViewport);

            std::array const Scale {
                    2.F / DisplaySize.x,
                    2.F / DisplaySize.y};

            std::array const Translation {
                    -1.F - DisplayPosition.x * Scale.at(0U),
                    -1.F - DisplayPosition.y * Scale.at(1U)};

            auto const ScaleSize       = static_cast<std::uint32_t>(sizeof(float) * Scale.size());
            auto const TranslationSize = static_cast<std::uint32_t>(sizeof(float) * Translation.size());

            vkCmdPushConstants(MainCommandBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, PushConstantOffset, ScaleSize, Scale.data());
            PushConstantOffset += ScaleSize;

            vkCmdPushConstants(MainCommandBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, PushConstantOffset, TranslationSize, Translation.data());
            PushConstantOffset += TranslationSize;

            std::span const ImGuiCmdListSpan(ImGuiDrawData->CmdLists.Data, ImGuiDrawData->CmdListsCount);

            std::uint32_t VertexBufferOffset {0U};
            std::uint32_t IndexBufferOffset {0U};

            for (std::vector const ImGuiCmdList(ImGuiCmdListSpan.begin(), ImGuiCmdListSpan.end());
                 ImDrawList const* const ImGuiCmdListIter: ImGuiCmdList)
            {
                ObjectAllocation& NewAllocation = m_ImGuiFontsAllocation.emplace_back();

                bool ImGuiVertexBound = false;
                // Bind Vertex Buffers
                {
                    VkDeviceSize const AllocationSize = ImGuiCmdListIter->VtxBuffer.Size * sizeof(ImDrawVert);

                    std::vector<Vertex> Vertices(ImGuiCmdListIter->VtxBuffer.Size);
                    for (ImDrawVert const& ImGuiVertexIter: ImGuiCmdListIter->VtxBuffer)
                    {
                        Vertices.push_back(Vertex {
                                .Position          = {ImGuiVertexIter.pos.x, ImGuiVertexIter.pos.y, 0.F},
                                .Color             = {static_cast<float>(ImGuiVertexIter.col), static_cast<float>(ImGuiVertexIter.col), static_cast<float>(ImGuiVertexIter.col)},
                                .TextureCoordinate = {ImGuiVertexIter.uv.x, ImGuiVertexIter.uv.y}});
                    }

                    if (BufferManager::Get().CreateVertexBuffer(NewAllocation, AllocationSize, Vertices);
                        NewAllocation.VertexBuffer.Buffer != VK_NULL_HANDLE)
                    {
                        vkCmdBindVertexBuffers(MainCommandBuffer, 0U, 1U, &NewAllocation.VertexBuffer.Buffer, Offsets.data());
                        ImGuiVertexBound = true;
                    }
                }

                bool ImGuiIndexBound = false;
                // Bind Index Buffers
                {
                    VkDeviceSize const AllocationSize = ImGuiCmdListIter->IdxBuffer.Size * sizeof(ImDrawIdx);

                    std::vector<std::uint32_t> Indices(ImGuiCmdListIter->IdxBuffer.Size);
                    for (ImDrawIdx const& ImGuiIndexIter: ImGuiCmdListIter->IdxBuffer)
                    {
                        Indices.push_back(ImGuiIndexIter);
                    }

                    if (BufferManager::Get().CreateIndexBuffer(NewAllocation, AllocationSize, Indices);
                        NewAllocation.IndexBuffer.Buffer != VK_NULL_HANDLE)
                    {
                        vkCmdBindIndexBuffer(MainCommandBuffer, NewAllocation.IndexBuffer.Buffer, 0U, VK_INDEX_TYPE_UINT32);
                        ImGuiIndexBound = true;
                    }
                }

                std::span const ImGuiCmdBufferSpan(ImGuiCmdListIter->CmdBuffer.Data, ImGuiCmdListIter->CmdBuffer.Size);
                for (std::vector const ImGuiCmdBuffers(ImGuiCmdBufferSpan.begin(), ImGuiCmdBufferSpan.end());
                     ImDrawCmd ImGuiCmdBufferIter: ImGuiCmdBuffers)
                {
                    if (ImGuiCmdBufferIter.UserCallback != nullptr)
                    {
                        ImGuiCmdBufferIter.UserCallback(ImGuiCmdListIter, &ImGuiCmdBufferIter);
                    }
                    else
                    {
                        ImVec2 ClipMin((ImGuiCmdBufferIter.ClipRect.x - DisplayPosition.x) * BufferScale.x, (ImGuiCmdBufferIter.ClipRect.y - DisplayPosition.y) * BufferScale.y);
                        ImVec2 ClipMax((ImGuiCmdBufferIter.ClipRect.z - DisplayPosition.x) * BufferScale.x, (ImGuiCmdBufferIter.ClipRect.w - DisplayPosition.y) * BufferScale.y);

                        if (ClipMin.x < 0.F)
                        {
                            ClipMin.x = 0.F;
                        }

                        if (ClipMax.x > static_cast<float>(WidthScale))
                        {
                            ClipMax.x = static_cast<float>(WidthScale);
                        }

                        if (ClipMin.y < 0.F)
                        {
                            ClipMin.y = 0.F;
                        }

                        if (ClipMax.y > static_cast<float>(HeightScale))
                        {
                            ClipMax.y = static_cast<float>(HeightScale);
                        }

                        if (ClipMax.x <= ClipMin.x || ClipMax.y <= ClipMin.y)
                        {
                            continue;
                        }

                        {
                            VkRect2D const ImGuiScissor {
                                    .offset {.x = static_cast<std::int32_t>(ClipMin.x),
                                             .y = static_cast<std::int32_t>(ClipMin.y)},
                                    .extent {.width  = static_cast<std::uint32_t>(ClipMax.x - ClipMin.x),
                                             .height = static_cast<std::uint32_t>(ClipMax.y - ClipMin.y)}};

                            vkCmdSetScissor(MainCommandBuffer, 0U, 1U, &ImGuiScissor);
                        }

                        if (ImGuiVertexBound && ImGuiIndexBound)
                        {
                            vkCmdDrawIndexed(
                                    MainCommandBuffer,
                                    static_cast<std::uint32_t>(ImGuiCmdBufferIter.ElemCount),
                                    1U,
                                    static_cast<std::uint32_t>(ImGuiCmdBufferIter.IdxOffset + IndexBufferOffset),
                                    static_cast<std::int32_t>(ImGuiCmdBufferIter.VtxOffset + VertexBufferOffset),
                                    0U);
                        }
                    }
                }

                VertexBufferOffset = ImGuiCmdListIter->VtxBuffer.Size;
                IndexBufferOffset  = ImGuiCmdListIter->IdxBuffer.Size;
            }

            VkRect2D const ImGuiScissor {
                    .offset {.x = 0U,
                             .y = 0U},
                    .extent {.width  = static_cast<std::uint32_t>(WidthScale),
                             .height = static_cast<std::uint32_t>(HeightScale)}};

            vkCmdSetScissor(MainCommandBuffer, 0, 1, &ImGuiScissor);
        }
    }

    if (ActiveRenderPass)
    {
        vkCmdEndRenderPass(MainCommandBuffer);
    }

    Helpers::CheckVulkanResult(vkEndCommandBuffer(MainCommandBuffer));
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
            .commandBufferCount   = static_cast<std::uint32_t>(m_CommandBuffers.size()),
            .pCommandBuffers      = m_CommandBuffers.data(),
            .signalSemaphoreCount = 1U,
            .pSignalSemaphores    = &m_RenderFinishedSemaphore};

    VkQueue const& GraphicsQueue = DeviceManager::Get().GetGraphicsQueue().second;

    Helpers::CheckVulkanResult(vkQueueSubmit(GraphicsQueue, 1U, &SubmitInfo, m_Fence));
    Helpers::CheckVulkanResult(vkQueueWaitIdle(GraphicsQueue));

    FreeCommandBuffers();
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
}

void CommandsManager::CreateGraphicsCommandPool()
{
    m_CommandPool = CreateCommandPool(DeviceManager::Get().GetGraphicsQueue().first);
}

void CommandsManager::AllocateCommandBuffer()
{
    VkDevice const& VulkanLogicalDevice = DeviceManager::Get().GetLogicalDevice();

    for (VkCommandBuffer& CommandBufferIter: m_CommandBuffers)
    {
        if (CommandBufferIter != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(VulkanLogicalDevice, m_CommandPool, 1U, &CommandBufferIter);
            CommandBufferIter = VK_NULL_HANDLE;
        }
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
            .commandBufferCount = static_cast<std::uint32_t>(m_CommandBuffers.size())};

    Helpers::CheckVulkanResult(vkAllocateCommandBuffers(VulkanLogicalDevice, &CommandBufferAllocateInfo, m_CommandBuffers.data()));
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

void CommandsManager::FreeCommandBuffers()
{
    vkFreeCommandBuffers(DeviceManager::Get().GetLogicalDevice(), m_CommandPool, static_cast<std::uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());

    for (VkCommandBuffer& CommandBufferIter: m_CommandBuffers)
    {
        CommandBufferIter = VK_NULL_HANDLE;
    }
}