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
import RenderCore.Management.ImGuiManagement;
import RenderCore.Types.AllocationTypes;
import RenderCore.Types.Camera;
import RenderCore.Types.Object;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import RenderCore.Utils.EnumConverter;
import Timer.ExecutionCounter;

using namespace RenderCore;

VkCommandPool g_CommandPool{};
std::vector<VkCommandBuffer> g_CommandBuffers{};
VkSemaphore g_ImageAvailableSemaphore{};
VkSemaphore g_RenderFinishedSemaphore{};
VkFence g_Fence{};

void AllocateCommandBuffer(std::uint32_t const QueueFamily, std::uint8_t const NumberOfBuffers)
{
    if (!std::empty(g_CommandBuffers))
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
    g_CommandBuffers.resize(NumberOfBuffers);

    VkCommandBufferAllocateInfo const CommandBufferAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = g_CommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
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
    VkCommandPoolCreateInfo const CommandPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = static_cast<std::uint32_t>(FamilyQueueIndex)};

    VkCommandPool Output = VK_NULL_HANDLE;
    CheckVulkanResult(vkCreateCommandPool(volkGetLoadedDevice(), &CommandPoolCreateInfo, nullptr, &Output));

    return Output;
}

void RenderCore::CreateCommandsSynchronizationObjects()
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan synchronization objects";

    constexpr VkSemaphoreCreateInfo SemaphoreCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    constexpr VkFenceCreateInfo FenceCreateInfo{
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

void RenderCore::RecordCommandBuffers(std::uint32_t const ImageIndex,
                                      Camera const& Camera,
                                      BufferManager const& BufferManager,
                                      PipelineManager const& PipelineManager,
                                      std::vector<std::shared_ptr<Object> > const& Objects,
                                      VkExtent2D const& SwapChainExtent)
{
    constexpr std::array<VkDeviceSize, 1U> Offsets{0U};

    constexpr VkImageLayout UndefinedLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    constexpr VkImageAspectFlags ImageAspect = VK_IMAGE_ASPECT_COLOR_BIT;
    constexpr VkImageAspectFlags DepthAspect = VK_IMAGE_ASPECT_DEPTH_BIT;

    constexpr VkImageLayout ViewportTransferLayout  = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    constexpr VkImageLayout ViewportFinalLayout     = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;
    constexpr VkImageLayout SwapChainLayout         = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    constexpr VkImageLayout SwapChainTransferLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    constexpr VkImageLayout SwapChainPresentLayout  = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    constexpr VkImageLayout DepthLayout             = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;

    constexpr VkCommandBufferBeginInfo CommandBufferBeginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    AllocateCommandBuffer(GetGraphicsQueue().first, 3U);
    VkPipelineLayout const& PipelineLayout = PipelineManager.GetPipelineLayout();

    const auto& [SwapChainImage, SwapChainView, SwapChainAllocation, SwapChainType] = BufferManager.GetSwapChainImages().at(ImageIndex);
    VkFormat const SwapChainFormat                                                  = BufferManager.GetSwapChainImageFormat();

    // Scene rendering commands
    {
        VkCommandBuffer& CommandBuffer = g_CommandBuffers.at(0U);
        CheckVulkanResult(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));

        VkPipeline const Pipeline = PipelineManager.GetMainPipeline();

        BufferManager::MoveImageLayout<UndefinedLayout, SwapChainLayout, ImageAspect>(CommandBuffer,
                                                                                      SwapChainImage,
                                                                                      SwapChainFormat);

        BufferManager::MoveImageLayout<UndefinedLayout, DepthLayout, DepthAspect>(CommandBuffer,
                                                                                  BufferManager.GetDepthImage().Image,
                                                                                  BufferManager.GetDepthFormat());

        VkRenderingAttachmentInfoKHR const ColorAttachmentInfo{
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                .imageView = SwapChainView,
                .imageLayout = SwapChainLayout,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = g_ClearValues.at(0U)};

        VkRenderingAttachmentInfoKHR const DepthAttachmentInfo{
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                .imageView = BufferManager.GetDepthImageView(),
                .imageLayout = DepthLayout,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = g_ClearValues.at(1U)};

        VkRenderingInfo const RenderingInfo{
                .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
                .renderArea = {
                        .offset = {0, 0},
                        .extent = SwapChainExtent},
                .layerCount = 1U,
                .colorAttachmentCount = 1U,
                .pColorAttachments = &ColorAttachmentInfo,
                .pDepthAttachment = &DepthAttachmentInfo};

        VkViewport const Viewport{
                .x = 0.F,
                .y = 0.F,
                .width = static_cast<float>(SwapChainExtent.width),
                .height = static_cast<float>(SwapChainExtent.height),
                .minDepth = 0.F,
                .maxDepth = 1.F};

        VkRect2D const Scissor{
                .offset = {0, 0},
                .extent = SwapChainExtent};

        vkCmdSetViewport(CommandBuffer, 0U, 1U, &Viewport);
        vkCmdSetScissor(CommandBuffer, 0U, 1U, &Scissor);

        vkCmdBeginRendering(CommandBuffer, &RenderingInfo);
        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);

        for (std::shared_ptr<Object> const& ObjectIter: Objects)
        {
            if (!ObjectIter || ObjectIter->IsPendingDestroy() || !Camera.CanDrawObject(ObjectIter, SwapChainExtent))
            {
                continue;
            }

            std::uint32_t const ObjectID = ObjectIter->GetID();

            if (VkDescriptorSet const& DescriptorSet = PipelineManager.GetDescriptorSet(ObjectID); DescriptorSet != VK_NULL_HANDLE)
            {
                vkCmdBindDescriptorSets(CommandBuffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        PipelineLayout,
                                        0U,
                                        1U,
                                        &DescriptorSet,
                                        0U,
                                        nullptr);
            }
            else
            {
                continue;
            }

            VkBuffer const& VertexBuffer   = BufferManager.GetVertexBuffer(ObjectID);
            VkBuffer const& IndexBuffer    = BufferManager.GetIndexBuffer(ObjectID);
            std::uint32_t const IndexCount = BufferManager.GetIndicesCount(ObjectID);

            BufferManager.UpdateUniformBuffers(ObjectIter, Camera, SwapChainExtent);

            bool ActiveVertexBinding{false};
            if (VertexBuffer != VK_NULL_HANDLE)
            {
                vkCmdBindVertexBuffers(CommandBuffer, 0U, 1U, &VertexBuffer, std::data(Offsets));
                ActiveVertexBinding = true;
            }

            bool ActiveIndexBinding{false};
            if (IndexBuffer != VK_NULL_HANDLE)
            {
                vkCmdBindIndexBuffer(CommandBuffer, IndexBuffer, 0U, VK_INDEX_TYPE_UINT32);
                ActiveIndexBinding = true;
            }

            if (ActiveVertexBinding && ActiveIndexBinding)
            {
                vkCmdDrawIndexed(CommandBuffer, IndexCount, 1U, 0U, 0U, 0U);
            }
        }

        vkCmdEndRendering(CommandBuffer);
        CheckVulkanResult(vkEndCommandBuffer(CommandBuffer));
    }

    // Viewport rendering commands
    {
        VkCommandBuffer& CommandBuffer = g_CommandBuffers.at(1U);
        CheckVulkanResult(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));

        const auto& [Image, View, Allocation, Type] = BufferManager.GetViewportImages().at(ImageIndex);

        VkPipeline const Pipeline           = PipelineManager.GetViewportPipeline();
        VkFormat const SwapChainImageFormat = BufferManager.GetSwapChainImageFormat();

        BufferManager::MoveImageLayout<SwapChainLayout, SwapChainTransferLayout, ImageAspect>(CommandBuffer,
                                                                                              SwapChainImage,
                                                                                              SwapChainImageFormat);

        BufferManager::MoveImageLayout<UndefinedLayout, ViewportTransferLayout, ImageAspect>(CommandBuffer,
                                                                                             Image,
                                                                                             SwapChainFormat);

        VkImageCopy const ImageCopy{
                .srcSubresource = {
                        .aspectMask = ImageAspect,
                        .mipLevel = 0U,
                        .baseArrayLayer = 0U,
                        .layerCount = 1U},
                .srcOffset = {0, 0, 0},
                .dstSubresource = {
                        .aspectMask = ImageAspect,
                        .mipLevel = 0U,
                        .baseArrayLayer = 0U,
                        .layerCount = 1U},
                .dstOffset = {0, 0, 0},
                .extent = {
                        .width = SwapChainExtent.width,
                        .height = SwapChainExtent.height,
                        .depth = 1U}};

        vkCmdCopyImage(CommandBuffer,
                       SwapChainImage,
                       SwapChainTransferLayout,
                       Image,
                       ViewportTransferLayout,
                       1U,
                       &ImageCopy);

        BufferManager::MoveImageLayout<ViewportTransferLayout, ViewportFinalLayout, ImageAspect>(CommandBuffer,
                                                                                                 Image,
                                                                                                 SwapChainFormat);

        VkRenderingAttachmentInfoKHR const ColorAttachmentInfo{
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                .imageView = View,
                .imageLayout = ViewportFinalLayout,
                .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE};

        VkRenderingAttachmentInfoKHR const DepthAttachmentInfo{
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                .imageView = BufferManager.GetDepthImageView(),
                .imageLayout = DepthLayout,
                .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE};

        VkRenderingInfo const RenderingInfo{
                .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
                .renderArea = {
                        .offset = {0, 0},
                        .extent = SwapChainExtent},
                .layerCount = 1U,
                .colorAttachmentCount = 1U,
                .pColorAttachments = &ColorAttachmentInfo,
                .pDepthAttachment = &DepthAttachmentInfo};

        VkViewport const Viewport{
                .x = 0.F,
                .y = 0.F,
                .width = static_cast<float>(SwapChainExtent.width),
                .height = static_cast<float>(SwapChainExtent.height),
                .minDepth = 0.F,
                .maxDepth = 1.F};

        VkRect2D const Scissor{
                .offset = {0, 0},
                .extent = SwapChainExtent};

        vkCmdSetViewport(CommandBuffer, 0U, 1U, &Viewport);
        vkCmdSetScissor(CommandBuffer, 0U, 1U, &Scissor);

        vkCmdBeginRendering(CommandBuffer, &RenderingInfo);
        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
        vkCmdEndRendering(CommandBuffer);

        CheckVulkanResult(vkEndCommandBuffer(CommandBuffer));
    }

    // ImGui rendering commands
    {
        VkCommandBuffer& CommandBuffer = g_CommandBuffers.at(2U);
        CheckVulkanResult(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));

        if (IsImGuiInitialized())
        {
            if (ImDrawData* const ImGuiDrawData = ImGui::GetDrawData())
            {
                VkRenderingAttachmentInfoKHR const ColorAttachmentInfo{
                        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                        .imageView = SwapChainView,
                        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
                        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
                        .storeOp = VK_ATTACHMENT_STORE_OP_STORE};

                VkRenderingInfo const RenderingInfo{
                        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
                        .renderArea = {
                                .offset = {0, 0},
                                .extent = SwapChainExtent},
                        .layerCount = 1U,
                        .colorAttachmentCount = 1U,
                        .pColorAttachments = &ColorAttachmentInfo};

                vkCmdBeginRendering(CommandBuffer, &RenderingInfo);
                ImGui_ImplVulkan_RenderDrawData(ImGuiDrawData, CommandBuffer);
                vkCmdEndRendering(CommandBuffer);
            }
        }

        BufferManager::MoveImageLayout<SwapChainTransferLayout, SwapChainPresentLayout, ImageAspect>(CommandBuffer,
                                                                                                     SwapChainImage,
                                                                                                     SwapChainFormat);

        CheckVulkanResult(vkEndCommandBuffer(CommandBuffer));
    }
}

void RenderCore::SubmitCommandBuffers()
{
    WaitAndResetFences();

    VkSemaphoreSubmitInfoKHR WaitSemaphoreInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
            .semaphore = g_ImageAvailableSemaphore,
            .value = 1U,
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR
    };

    std::vector<VkCommandBufferSubmitInfoKHR> CommandBufferInfos;
    CommandBufferInfos.reserve(std::size(g_CommandBuffers));

    for (VkCommandBuffer const& CommandBufferIter: g_CommandBuffers)
    {
        CommandBufferInfos.push_back({
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR,
                .commandBuffer = CommandBufferIter});
    }

    VkSemaphoreSubmitInfoKHR SignalSemaphoreInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
            .semaphore = g_RenderFinishedSemaphore,
            .value = 1U
    };

    VkSubmitInfo2KHR const SubmitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR,
            .waitSemaphoreInfoCount = 1U,
            .pWaitSemaphoreInfos = &WaitSemaphoreInfo,
            .commandBufferInfoCount = static_cast<std::uint32_t>(std::size(CommandBufferInfos)),
            .pCommandBufferInfos = std::data(CommandBufferInfos),
            .signalSemaphoreInfoCount = 1U,
            .pSignalSemaphoreInfos = &SignalSemaphoreInfo
    };

    auto const& GraphicsQueue = GetGraphicsQueue().second;

    CheckVulkanResult(vkQueueSubmit2(GraphicsQueue, 1U, &SubmitInfo, g_Fence));
    CheckVulkanResult(vkQueueWaitIdle(GraphicsQueue));

    FreeCommandBuffers();
}

void RenderCore::PresentFrame(std::uint32_t const ImageIndice, VkSwapchainKHR const& SwapChain)
{
    VkPresentInfoKHR const PresentInfo{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1U,
            .pWaitSemaphores = &g_RenderFinishedSemaphore,
            .swapchainCount = 1U,
            .pSwapchains = &SwapChain,
            .pImageIndices = &ImageIndice,
            .pResults = nullptr};

    auto const& Queue = GetGraphicsQueue().second;

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
    VkCommandPoolCreateInfo const CommandPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = static_cast<std::uint32_t>(QueueFamilyIndex)};

    CheckVulkanResult(vkCreateCommandPool(volkGetLoadedDevice(), &CommandPoolCreateInfo, nullptr, &CommandPool));

    constexpr VkCommandBufferBeginInfo CommandBufferBeginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    VkCommandBufferAllocateInfo const CommandBufferAllocateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = CommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
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

    VkSubmitInfo const SubmitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = static_cast<std::uint32_t>(std::size(CommandBuffers)),
            .pCommandBuffers = std::data(CommandBuffers),
    };

    CheckVulkanResult(vkQueueSubmit(Queue, 1U, &SubmitInfo, VK_NULL_HANDLE));
    CheckVulkanResult(vkQueueWaitIdle(Queue));

    vkFreeCommandBuffers(volkGetLoadedDevice(), CommandPool, static_cast<std::uint32_t>(std::size(CommandBuffers)), std::data(CommandBuffers));
    vkDestroyCommandPool(volkGetLoadedDevice(), CommandPool, nullptr);
}
