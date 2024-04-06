// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <Volk/volk.h>
#include <algorithm>
#include <memory>
#include <vector>

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#endif

module RenderCore.Runtime.Command;

import RenderCore.Runtime.Synchronization;
import RenderCore.Runtime.Scene;
import RenderCore.Runtime.SwapChain;
import RenderCore.Runtime.Memory;
import RenderCore.Runtime.Device;
import RenderCore.Runtime.Pipeline;
import RenderCore.Integrations.Viewport;
import RenderCore.Integrations.ImGuiOverlay;
import RenderCore.Types.Allocation;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;

using namespace RenderCore;

VkCommandPool                g_CommandPool {};
std::vector<VkCommandBuffer> g_CommandBuffers {};

void AllocateCommandBuffers(std::uint32_t const QueueFamily, std::uint8_t const NumberOfBuffers)
{
    if (!std::empty(g_CommandBuffers))
    {
        vkFreeCommandBuffers(volkGetLoadedDevice(),
                             g_CommandPool,
                             static_cast<std::uint32_t>(std::size(g_CommandBuffers)),
                             std::data(g_CommandBuffers));
        g_CommandBuffers.clear();
    }

    if (g_CommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(volkGetLoadedDevice(), g_CommandPool, nullptr);
        g_CommandPool = VK_NULL_HANDLE;
    }

    g_CommandPool = CreateCommandPool(QueueFamily);
    g_CommandBuffers.resize(NumberOfBuffers);

    VkCommandBufferAllocateInfo const CommandBufferAllocateInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = g_CommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<std::uint32_t>(std::size(g_CommandBuffers))
    };

    CheckVulkanResult(vkAllocateCommandBuffers(volkGetLoadedDevice(), &CommandBufferAllocateInfo, std::data(g_CommandBuffers)));
}

void FreeCommandBuffers()
{
    if (!std::empty(g_CommandBuffers))
    {
        vkFreeCommandBuffers(volkGetLoadedDevice(),
                             g_CommandPool,
                             static_cast<std::uint32_t>(std::size(g_CommandBuffers)),
                             std::data(g_CommandBuffers));

        std::ranges::for_each(g_CommandBuffers,
                              [](VkCommandBuffer &CommandBufferIter)
                              {
                                  CommandBufferIter = VK_NULL_HANDLE;
                              });
        g_CommandBuffers.clear();
    }
}

void RenderCore::ReleaseCommandsResources()
{
    FreeCommandBuffers();

    if (g_CommandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(volkGetLoadedDevice(), g_CommandPool, nullptr);
        g_CommandPool = VK_NULL_HANDLE;
    }
}

VkCommandPool RenderCore::CreateCommandPool(std::uint8_t const FamilyQueueIndex)
{
    VkCommandPoolCreateInfo const CommandPoolCreateInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = static_cast<std::uint32_t>(FamilyQueueIndex)
    };

    VkCommandPool Output = VK_NULL_HANDLE;
    CheckVulkanResult(vkCreateCommandPool(volkGetLoadedDevice(), &CommandPoolCreateInfo, nullptr, &Output));

    return Output;
}

constexpr VkImageAspectFlags ImageAspect = VK_IMAGE_ASPECT_COLOR_BIT;
constexpr VkImageAspectFlags DepthAspect = VK_IMAGE_ASPECT_DEPTH_BIT;

constexpr VkImageLayout UndefinedLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
constexpr VkImageLayout SwapChainMidLayout   = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
constexpr VkImageLayout SwapChainFinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
constexpr VkImageLayout DepthLayout          = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;

void BindDescriptorSets(VkCommandBuffer const &CommandBuffer)
{
    for (Object const ObjectIter : GetObjects())
    {
        if (GetCamera().CanDrawObject(ObjectIter))
        {
            ObjectIter.UpdateUniformBuffers();
            ObjectIter.DrawObject(CommandBuffer);
        }
    }
}

void SetViewport(VkCommandBuffer const &CommandBuffer, VkExtent2D const &SwapChainExtent)
{
    VkViewport const Viewport {
            .x = 0.F,
            .y = 0.F,
            .width = static_cast<float>(SwapChainExtent.width),
            .height = static_cast<float>(SwapChainExtent.height),
            .minDepth = 0.F,
            .maxDepth = 1.F
    };

    VkRect2D const Scissor { .offset = { 0, 0 }, .extent = SwapChainExtent };

    vkCmdSetViewport(CommandBuffer, 0U, 1U, &Viewport);
    vkCmdSetScissor(CommandBuffer, 0U, 1U, &Scissor);
}

void RecordSceneCommands(VkCommandBuffer &CommandBuffer, std::uint32_t const ImageIndex, VkExtent2D const &SwapChainExtent)
{
    SetViewport(CommandBuffer, SwapChainExtent);
    VkFormat const SwapChainFormat = GetSwapChainImageFormat();

    std::vector<VkRenderingAttachmentInfoKHR> ColorAttachments {};
    ColorAttachments.reserve(2U);

    #ifdef VULKAN_RENDERER_ENABLE_IMGUI
    constexpr VkImageLayout ViewportMidLayout   = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    constexpr VkImageLayout ViewportFinalLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;

    ImageAllocation const &ViewportAllocation = GetViewportImages().at(ImageIndex);
    RenderCore::MoveImageLayout<UndefinedLayout, ViewportMidLayout, ImageAspect>(CommandBuffer, ViewportAllocation.Image, SwapChainFormat);

    ColorAttachments.push_back(VkRenderingAttachmentInfoKHR {
                                       .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                                       .imageView = ViewportAllocation.View,
                                       .imageLayout = ViewportMidLayout,
                                       .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                       .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                                       .clearValue = g_ClearValues.at(0U)
                               });
    #endif

    ImageAllocation const &SwapchainAllocation = GetSwapChainImages().at(ImageIndex);
    RenderCore::MoveImageLayout<UndefinedLayout, SwapChainMidLayout, ImageAspect>(CommandBuffer, SwapchainAllocation.Image, SwapChainFormat);

    ColorAttachments.push_back(VkRenderingAttachmentInfoKHR {
                                       .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                                       #ifdef VULKAN_RENDERER_ENABLE_IMGUI
                                       .imageView = ViewportAllocation.View,
                                       .imageLayout = ViewportMidLayout,
                                       .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                       .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                                       .clearValue = g_ClearValues.at(1U)
                                       #else
                                       .imageView = SwapchainAllocation.View,
                                       .imageLayout = SwapChainMidLayout,
                                       .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                       .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                                       .clearValue = g_ClearValues.at(0U)
                                       #endif
                               });

    ColorAttachments.shrink_to_fit();

    ImageAllocation const &DepthAllocation = GetDepthImage();
    RenderCore::MoveImageLayout<UndefinedLayout, DepthLayout, DepthAspect>(CommandBuffer, DepthAllocation.Image, DepthAllocation.Format);

    VkRenderingAttachmentInfoKHR const DepthAttachmentInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .imageView = DepthAllocation.View,
            .imageLayout = DepthLayout,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = g_ClearValues.at(2U)
    };

    VkRenderingInfo const RenderingInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
            .renderArea = { .offset = { 0, 0 }, .extent = SwapChainExtent },
            .layerCount = 1U,
            .colorAttachmentCount = static_cast<std::uint32_t>(std::size(ColorAttachments)),
            .pColorAttachments = std::data(ColorAttachments),
            .pDepthAttachment = &DepthAttachmentInfo,
            .pStencilAttachment = &DepthAttachmentInfo
    };

    vkCmdBeginRendering(CommandBuffer, &RenderingInfo);
    {
        VkPipeline const Pipeline = GetMainPipeline();
        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
        BindDescriptorSets(CommandBuffer);
    }
    vkCmdEndRendering(CommandBuffer);

    #ifdef VULKAN_RENDERER_ENABLE_IMGUI
    RenderCore::MoveImageLayout<ViewportMidLayout, ViewportFinalLayout, ImageAspect>(CommandBuffer, ViewportAllocation.Image, SwapChainFormat);

    if (!IsImGuiInitialized())
    {
        RenderCore::MoveImageLayout<SwapChainMidLayout, SwapChainFinalLayout, ImageAspect>(CommandBuffer, SwapchainAllocation.Image, SwapChainFormat);
    }
    #else
    RenderCore::MoveImageLayout<SwapChainMidLayout, SwapChainFinalLayout, ImageAspect>(CommandBuffer, SwapchainAllocation.Image, SwapChainFormat);
    #endif
}

void RenderCore::RecordCommandBuffers(std::uint32_t const ImageIndex)
{
    AllocateCommandBuffers(GetGraphicsQueue().first, 1U);
    VkCommandBuffer &CommandBuffer = g_CommandBuffers.at(0U);

    constexpr VkCommandBufferBeginInfo CommandBufferBeginInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    CheckVulkanResult(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));
    {
        VkExtent2D const &SwapChainExtent = GetSwapChainExtent();
        RecordSceneCommands(CommandBuffer, ImageIndex, SwapChainExtent);

        #ifdef VULKAN_RENDERER_ENABLE_IMGUI
        if (IsImGuiInitialized())
        {
            ImageAllocation const &SwapchainAllocation = GetSwapChainImages().at(ImageIndex);
            VkFormat const         SwapChainFormat     = GetSwapChainImageFormat();

            if (ImDrawData *const ImGuiDrawData = ImGui::GetDrawData())
            {
                VkRenderingAttachmentInfoKHR const ColorAttachmentInfo {
                        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                        .imageView = SwapchainAllocation.View,
                        .imageLayout = SwapChainMidLayout,
                        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                        .clearValue = g_ClearValues.at(0U)
                };

                VkRenderingInfo const RenderingInfo {
                        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
                        .renderArea = { .offset = { 0, 0 }, .extent = SwapChainExtent },
                        .layerCount = 1U,
                        .colorAttachmentCount = 1U,
                        .pColorAttachments = &ColorAttachmentInfo
                };

                vkCmdBeginRendering(CommandBuffer, &RenderingInfo);
                ImGui_ImplVulkan_RenderDrawData(ImGuiDrawData, CommandBuffer);
                vkCmdEndRendering(CommandBuffer);
            }

            RenderCore::MoveImageLayout<SwapChainMidLayout, SwapChainFinalLayout, ImageAspect>(CommandBuffer,
                                                                                               SwapchainAllocation.Image,
                                                                                               SwapChainFormat);
        }
        #endif
    }
    CheckVulkanResult(vkEndCommandBuffer(CommandBuffer));
}

void RenderCore::SubmitCommandBuffers()
{
    VkSemaphoreSubmitInfoKHR WaitSemaphoreInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
            .semaphore = GetImageAvailableSemaphore(),
            .value = 1U,
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
            .deviceIndex = 0U
    };

    VkSemaphoreSubmitInfoKHR SignalSemaphoreInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
            .semaphore = GetRenderFinishedSemaphore(),
            .value = 1U
    };

    std::vector<VkCommandBufferSubmitInfoKHR> CommandBufferInfos;
    CommandBufferInfos.reserve(std::size(g_CommandBuffers));

    for (VkCommandBuffer const &CommandBufferIter : g_CommandBuffers)
    {
        CommandBufferInfos.push_back(VkCommandBufferSubmitInfo {
                                             .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR,
                                             .commandBuffer = CommandBufferIter,
                                             .deviceMask = 0U
                                     });
    }

    VkSubmitInfo2KHR const SubmitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR,
            .waitSemaphoreInfoCount = 1U,
            .pWaitSemaphoreInfos = &WaitSemaphoreInfo,
            .commandBufferInfoCount = static_cast<std::uint32_t>(std::size(CommandBufferInfos)),
            .pCommandBufferInfos = std::data(CommandBufferInfos),
            .signalSemaphoreInfoCount = 1U,
            .pSignalSemaphoreInfos = &SignalSemaphoreInfo
    };

    auto const &GraphicsQueue = GetGraphicsQueue().second;

    CheckVulkanResult(vkQueueSubmit2(GraphicsQueue, 1U, &SubmitInfo, GetFence()));
    WaitAndResetFences();
    FreeCommandBuffers();
}

void RenderCore::InitializeSingleCommandQueue(VkCommandPool &               CommandPool,
                                              std::vector<VkCommandBuffer> &CommandBuffers,
                                              std::uint8_t const            QueueFamilyIndex)
{
    VkCommandPoolCreateInfo const CommandPoolCreateInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = static_cast<std::uint32_t>(QueueFamilyIndex)
    };

    CheckVulkanResult(vkCreateCommandPool(volkGetLoadedDevice(), &CommandPoolCreateInfo, nullptr, &CommandPool));

    constexpr VkCommandBufferBeginInfo CommandBufferBeginInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    VkCommandBufferAllocateInfo const CommandBufferAllocateInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = CommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<std::uint32_t>(std::size(CommandBuffers)),
    };

    CheckVulkanResult(vkAllocateCommandBuffers(volkGetLoadedDevice(), &CommandBufferAllocateInfo, std::data(CommandBuffers)));
    for (VkCommandBuffer const &CommandBufferIter : CommandBuffers)
    {
        CheckVulkanResult(vkBeginCommandBuffer(CommandBufferIter, &CommandBufferBeginInfo));
    }
}

void RenderCore::FinishSingleCommandQueue(VkQueue const &Queue, VkCommandPool const &CommandPool, std::vector<VkCommandBuffer> const &CommandBuffers)
{
    std::vector<VkCommandBufferSubmitInfoKHR> CommandBufferInfos;
    CommandBufferInfos.reserve(std::size(g_CommandBuffers));

    for (VkCommandBuffer const &CommandBufferIter : CommandBuffers)
    {
        CheckVulkanResult(vkEndCommandBuffer(CommandBufferIter));

        CommandBufferInfos.push_back(VkCommandBufferSubmitInfo {
                                             .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR,
                                             .commandBuffer = CommandBufferIter,
                                             .deviceMask = 0U
                                     });
    }

    VkSubmitInfo2KHR const SubmitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR,
            .commandBufferInfoCount = static_cast<std::uint32_t>(std::size(CommandBufferInfos)),
            .pCommandBufferInfos = std::data(CommandBufferInfos)
    };

    CheckVulkanResult(vkQueueSubmit2(Queue, 1U, &SubmitInfo, VK_NULL_HANDLE));
    CheckVulkanResult(vkQueueWaitIdle(Queue));

    vkFreeCommandBuffers(volkGetLoadedDevice(), CommandPool, static_cast<std::uint32_t>(std::size(CommandBuffers)), std::data(CommandBuffers));
    vkDestroyCommandPool(volkGetLoadedDevice(), CommandPool, nullptr);
}
