// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <algorithm>
#include <chrono>
#include <functional>
#include <ranges>
#include <thread>
#include <unordered_map>
#include <vector>
#include <Volk/volk.h>

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
import ThreadPool;

using namespace RenderCore;

constexpr VkImageAspectFlags g_ImageAspect = VK_IMAGE_ASPECT_COLOR_BIT;
constexpr VkImageAspectFlags g_DepthAspect = VK_IMAGE_ASPECT_DEPTH_BIT;

constexpr VkImageLayout g_UndefinedLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
constexpr VkImageLayout g_SwapChainMidLayout   = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
constexpr VkImageLayout g_SwapChainFinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
constexpr VkImageLayout g_DepthLayout          = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
constexpr VkImageLayout g_ViewportMidLayout   = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
constexpr VkImageLayout g_ViewportFinalLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
#endif

constexpr VkCommandBufferBeginInfo g_CommandBufferBeginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
};

struct ThreadResources
{
    VkCommandPool   CommandPool { VK_NULL_HANDLE };
    VkCommandBuffer CommandBuffer { VK_NULL_HANDLE };

    void Allocate(std::uint8_t const QueueFamilyIndex, std::uint8_t const NumberOfBuffers)
    {
        if (NumberOfBuffers == 0U)
        {
            return;
        }

        CommandPool = CreateCommandPool(QueueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        VkCommandBufferAllocateInfo const CommandBufferAllocateInfo {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = CommandPool,
                .level = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
                .commandBufferCount = 1U
        };

        VkDevice const &LogicalDevice = GetLogicalDevice();
        CheckVulkanResult(vkAllocateCommandBuffers(LogicalDevice, &CommandBufferAllocateInfo, &CommandBuffer));
    }

    void Free()
    {
        if (CommandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        VkDevice const &LogicalDevice = GetLogicalDevice();
        vkFreeCommandBuffers(LogicalDevice, CommandPool, 1U, &CommandBuffer);
        CommandBuffer = VK_NULL_HANDLE;
    }

    void Destroy()
    {
        if (CommandPool == VK_NULL_HANDLE)
        {
            return;
        }

        Free();
        VkDevice const &LogicalDevice = GetLogicalDevice();
        vkDestroyCommandPool(LogicalDevice, CommandPool, nullptr);
    }
};

std::uint32_t                                     g_ObjectsPerThread { 0U };
std::uint32_t                                     g_NumThreads { 0U };
ThreadPool::Pool                                  g_ThreadPool {};
std::unordered_map<std::uint8_t, ThreadResources> g_CommandResources {};
std::pair<VkCommandPool, VkCommandBuffer>         g_PrimaryCommandBuffer {};

void RenderCore::AllocateCommandBuffers(std::uint32_t const QueueFamily, std::uint32_t const NumObjects)
{
    if (NumObjects > 0U)
    {
        g_ObjectsPerThread = static_cast<std::uint32_t>(std::ceil(static_cast<float>(NumObjects) / static_cast<float>(g_NumThreads)));

        for (std::uint8_t ThreadIndex = 0U; ThreadIndex < g_NumThreads; ++ThreadIndex)
        {
            if (ThreadIndex * g_ObjectsPerThread >= NumObjects)
            {
                break;
            }

            std::uint32_t const ObjectsToAllocate = std::min(g_ObjectsPerThread, NumObjects - ThreadIndex * g_ObjectsPerThread);
            g_CommandResources.at(ThreadIndex).Allocate(QueueFamily, ObjectsToAllocate);
        }
    }

    {
        g_PrimaryCommandBuffer = std::make_pair(CreateCommandPool(QueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT), VK_NULL_HANDLE);

        VkCommandBufferAllocateInfo const CommandBufferAllocateInfo {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = g_PrimaryCommandBuffer.first,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1U
        };

        VkDevice const &LogicalDevice = GetLogicalDevice();
        CheckVulkanResult(vkAllocateCommandBuffers(LogicalDevice, &CommandBufferAllocateInfo, &g_PrimaryCommandBuffer.second));
    }
}

void RenderCore::FreeCommandBuffers()
{
    for (auto &CommandResources : g_CommandResources | std::views::values)
    {
        CommandResources.Free();
    }

    VkDevice const &LogicalDevice = GetLogicalDevice();
    vkFreeCommandBuffers(LogicalDevice, g_PrimaryCommandBuffer.first, 1U, &g_PrimaryCommandBuffer.second);
}

void RenderCore::InitializeCommandsResources()
{
    g_NumThreads = static_cast<std::uint8_t>(std::thread::hardware_concurrency());
    g_ThreadPool.SetupCPUThreads();

    for (std::uint8_t ThreadIndex = 0U; ThreadIndex < g_NumThreads; ++ThreadIndex)
    {
        g_CommandResources.emplace(ThreadIndex, ThreadResources {});
    }
}

void RenderCore::ReleaseCommandsResources()
{
    g_ThreadPool.Wait();

    for (auto &CommandResources : g_CommandResources | std::views::values)
    {
        CommandResources.Destroy();
    }
    g_CommandResources.clear();

    VkDevice const &LogicalDevice = GetLogicalDevice();
    vkDestroyCommandPool(LogicalDevice, g_PrimaryCommandBuffer.first, nullptr);
}

VkCommandPool RenderCore::CreateCommandPool(std::uint8_t const FamilyQueueIndex, VkCommandPoolCreateFlags const Flags)
{
    VkCommandPoolCreateInfo const CommandPoolCreateInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = Flags,
            .queueFamilyIndex = static_cast<std::uint32_t>(FamilyQueueIndex)
    };

    VkDevice const &LogicalDevice = GetLogicalDevice();

    VkCommandPool Output = VK_NULL_HANDLE;
    CheckVulkanResult(vkCreateCommandPool(LogicalDevice, &CommandPoolCreateInfo, nullptr, &Output));

    return Output;
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

void BeginRendering(std::uint32_t const ImageIndex, ImageAllocation const &SwapchainAllocation)
{
    ImageAllocation const &DepthAllocation = GetDepthImage();

    std::vector ImageBarriers {
            RenderCore::MountImageBarrier<g_UndefinedLayout, g_SwapChainMidLayout, g_ImageAspect>(SwapchainAllocation.Image,
                                                                                                  SwapchainAllocation.Format),
            RenderCore::MountImageBarrier<g_UndefinedLayout, g_DepthLayout, g_DepthAspect>(DepthAllocation.Image, DepthAllocation.Format)
    };

    #ifdef VULKAN_RENDERER_ENABLE_IMGUI
    ImageAllocation const &ViewportAllocation = GetViewportImages().at(ImageIndex);
    ImageBarriers.push_back(RenderCore::MountImageBarrier<g_UndefinedLayout, g_ViewportMidLayout, g_ImageAspect>(ViewportAllocation.Image,
                                SwapchainAllocation.Format));

    VkRenderingAttachmentInfo ColorAttachment {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = ViewportAllocation.View,
            .imageLayout = g_ViewportMidLayout,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = g_ClearValues.at(0U)
    };
    #else
    VkRenderingAttachmentInfo ColorAttachment {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = SwapchainAllocation.View,
        .imageLayout = g_SwapChainMidLayout,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = g_ClearValues.at(0U)
    };
    #endif

    VkDependencyInfo const DependencyInfo {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = static_cast<std::uint32_t>(std::size(ImageBarriers)),
            .pImageMemoryBarriers = std::data(ImageBarriers)
    };

    vkCmdPipelineBarrier2(g_PrimaryCommandBuffer.second, &DependencyInfo);

    VkRenderingAttachmentInfo const DepthAttachmentInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = DepthAllocation.View,
            .imageLayout = g_DepthLayout,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = g_ClearValues.at(1U)
    };

    VkRenderingInfo const RenderingInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .pNext = nullptr,
            .flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT,
            .renderArea = { .offset = { 0, 0 }, .extent = SwapchainAllocation.Extent },
            .layerCount = 1U,
            .colorAttachmentCount = 1U,
            .pColorAttachments = &ColorAttachment,
            .pDepthAttachment = &DepthAttachmentInfo,
            .pStencilAttachment = &DepthAttachmentInfo
    };

    vkCmdBeginRendering(g_PrimaryCommandBuffer.second, &RenderingInfo);
}

void EndRendering(std::uint32_t const ImageIndex)
{
    vkCmdEndRendering(g_PrimaryCommandBuffer.second);

    #ifdef VULKAN_RENDERER_ENABLE_IMGUI
    ImageAllocation const &ViewportAllocation = GetViewportImages().at(ImageIndex);
    RenderCore::RequestImageLayoutTransition<g_ViewportMidLayout, g_ViewportFinalLayout, g_ImageAspect>(g_PrimaryCommandBuffer.second,
        ViewportAllocation.Image,
        GetSwapChainImages().at(ImageIndex).Format);
    #endif
}

std::vector<VkCommandBuffer> RecordSceneCommands(ImageAllocation const &SwapchainAllocation)
{
    VkFormat const &DepthFormat = GetDepthImage().Format;

    VkCommandBufferInheritanceRenderingInfo const InheritanceRenderingInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO,
            .flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT,
            .viewMask = 0U,
            .colorAttachmentCount = 1U,
            .pColorAttachmentFormats = &SwapchainAllocation.Format,
            .depthAttachmentFormat = DepthFormat,
            .stencilAttachmentFormat = DepthFormat,
            .rasterizationSamples = g_MSAASamples,
    };

    VkCommandBufferInheritanceInfo const InheritanceInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
            .pNext = &InheritanceRenderingInfo
    };

    VkCommandBufferBeginInfo SecondaryBeginInfo = g_CommandBufferBeginInfo;
    SecondaryBeginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    SecondaryBeginInfo.pInheritanceInfo         = &InheritanceInfo;

    VkPipeline const &      Pipeline       = GetMainPipeline();
    VkPipelineLayout const &PipelineLayout = GetPipelineLayout();
    auto const &            Objects        = GetObjects();

    if (std::empty(Objects))
    {
        return {};
    }

    std::vector<VkCommandBuffer> Output {};
    Output.reserve(g_NumThreads);

    auto const &Camera = GetCamera();

    for (std::uint32_t ThreadIndex = 0U; ThreadIndex < g_NumThreads; ++ThreadIndex)
    {
        auto const &[CommandPool, CommandBuffer] = g_CommandResources.at(ThreadIndex);

        if (CommandBuffer == VK_NULL_HANDLE)
        {
            break;
        }

        g_ThreadPool.AddTask([CommandBuffer, ThreadIndex, &Objects, &Camera, &Pipeline, &PipelineLayout, &SecondaryBeginInfo, &SwapchainAllocation]
                             {
                                 CheckVulkanResult(vkBeginCommandBuffer(CommandBuffer, &SecondaryBeginInfo));
                                 {
                                     SetViewport(CommandBuffer, SwapchainAllocation.Extent);
                                     vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);

                                     for (std::uint32_t ObjectIndex = 0U; ObjectIndex < g_ObjectsPerThread; ++ObjectIndex)
                                     {
                                         std::uint32_t const ObjectAccessIndex = ThreadIndex * g_ObjectsPerThread + ObjectIndex;

                                         if (ObjectAccessIndex >= std::size(Objects))
                                         {
                                             break;
                                         }

                                         std::shared_ptr<Object> const &Object = Objects.at(ObjectAccessIndex);

                                         if (!Camera.CanDrawObject(Object))
                                         {
                                             continue;
                                         }

                                         Object->DrawObject(CommandBuffer, PipelineLayout, ObjectAccessIndex);
                                         Object->UpdateUniformBuffers();
                                     }
                                 }
                                 CheckVulkanResult(vkEndCommandBuffer(CommandBuffer));
                             },
                             ThreadIndex);

        Output.push_back(CommandBuffer);
    }

    g_ThreadPool.Wait();

    return Output;
}

void RenderCore::RecordCommandBuffers(std::uint32_t const ImageIndex)
{
    CheckVulkanResult(vkBeginCommandBuffer(g_PrimaryCommandBuffer.second, &g_CommandBufferBeginInfo));
    {
        ImageAllocation const &SwapchainAllocation = GetSwapChainImages().at(ImageIndex);

        BeginRendering(ImageIndex, SwapchainAllocation);

        if (std::vector<VkCommandBuffer> const CommandBuffers = RecordSceneCommands(SwapchainAllocation);
            !std::empty(CommandBuffers))
        {
            vkCmdExecuteCommands(g_PrimaryCommandBuffer.second, static_cast<std::uint32_t>(std::size(CommandBuffers)), std::data(CommandBuffers));
        }

        EndRendering(ImageIndex);

        #ifdef VULKAN_RENDERER_ENABLE_IMGUI
        RecordImGuiCommandBuffer(g_PrimaryCommandBuffer.second, SwapchainAllocation, g_SwapChainMidLayout);
        #endif

        RenderCore::RequestImageLayoutTransition<g_SwapChainMidLayout, g_SwapChainFinalLayout, g_ImageAspect>(g_PrimaryCommandBuffer.second,
            SwapchainAllocation.Image,
            SwapchainAllocation.Format);
    }
    CheckVulkanResult(vkEndCommandBuffer(g_PrimaryCommandBuffer.second));
}

void RenderCore::SubmitCommandBuffers()
{
    VkSemaphoreSubmitInfo const WaitSemaphoreInfo {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = GetImageAvailableSemaphore(),
            .value = 1U,
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSemaphoreSubmitInfo const SignalSemaphoreInfo {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = GetRenderFinishedSemaphore(),
            .value = 1U,
            .stageMask = VK_PIPELINE_STAGE_2_NONE
    };

    VkCommandBufferSubmitInfo const PrimarySubmission {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = g_PrimaryCommandBuffer.second
    };

    VkSubmitInfo2 const SubmitInfo {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .waitSemaphoreInfoCount = 1U,
            .pWaitSemaphoreInfos = &WaitSemaphoreInfo,
            .commandBufferInfoCount = 1U,
            .pCommandBufferInfos = &PrimarySubmission,
            .signalSemaphoreInfoCount = 1U,
            .pSignalSemaphoreInfos = &SignalSemaphoreInfo
    };

    auto const &GraphicsQueue = GetGraphicsQueue().second;

    CheckVulkanResult(vkQueueSubmit2(GraphicsQueue, 1U, &SubmitInfo, GetFence()));
    WaitAndResetFences();
}

void RenderCore::InitializeSingleCommandQueue(VkCommandPool &               CommandPool,
                                              std::vector<VkCommandBuffer> &CommandBuffers,
                                              std::uint8_t const            QueueFamilyIndex)
{
    if (std::empty(CommandBuffers))
    {
        return;
    }

    CommandPool = CreateCommandPool(QueueFamilyIndex, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

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

    VkDevice const &LogicalDevice = GetLogicalDevice();

    CheckVulkanResult(vkAllocateCommandBuffers(LogicalDevice, &CommandBufferAllocateInfo, std::data(CommandBuffers)));
    for (VkCommandBuffer const &CommandBufferIter : CommandBuffers)
    {
        CheckVulkanResult(vkBeginCommandBuffer(CommandBufferIter, &CommandBufferBeginInfo));
    }
}

void RenderCore::FinishSingleCommandQueue(VkQueue const &Queue, VkCommandPool const &CommandPool, std::vector<VkCommandBuffer> const &CommandBuffers)
{
    if (std::empty(CommandBuffers))
    {
        return;
    }

    std::vector<VkCommandBufferSubmitInfo> CommandBufferInfos;
    CommandBufferInfos.reserve(std::size(CommandBuffers));

    for (VkCommandBuffer const &CommandBufferIter : CommandBuffers)
    {
        CheckVulkanResult(vkEndCommandBuffer(CommandBufferIter));

        CommandBufferInfos.push_back(VkCommandBufferSubmitInfo {
                                             .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                                             .commandBuffer = CommandBufferIter,
                                             .deviceMask = 0U
                                     });
    }

    VkSubmitInfo2 const SubmitInfo {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .commandBufferInfoCount = static_cast<std::uint32_t>(std::size(CommandBufferInfos)),
            .pCommandBufferInfos = std::data(CommandBufferInfos)
    };

    CheckVulkanResult(vkQueueSubmit2(Queue, 1U, &SubmitInfo, VK_NULL_HANDLE));
    CheckVulkanResult(vkQueueWaitIdle(Queue));

    VkDevice const &LogicalDevice = GetLogicalDevice();

    vkFreeCommandBuffers(LogicalDevice, CommandPool, static_cast<std::uint32_t>(std::size(CommandBuffers)), std::data(CommandBuffers));
    vkDestroyCommandPool(LogicalDevice, CommandPool, nullptr);
}
