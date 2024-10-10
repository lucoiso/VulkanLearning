// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.Runtime.Command;

using namespace RenderCore;

import RenderCore.Renderer;
import RenderCore.Runtime.Device;
import RenderCore.Runtime.Pipeline;
import RenderCore.Runtime.Scene;
import RenderCore.Runtime.Synchronization;
import RenderCore.Runtime.Memory;
import RenderCore.Runtime.SwapChain;
import RenderCore.Runtime.Offscreen;
import RenderCore.Types.Camera;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;

constexpr VkCommandBufferBeginInfo g_CommandBufferBeginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
};

void ThreadResources::Allocate(VkDevice const &LogicalDevice, std::uint8_t const QueueFamilyIndex)
{
    CommandPool = CreateCommandPool(QueueFamilyIndex, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

    VkCommandBufferAllocateInfo const CommandBufferAllocateInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = CommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
            .commandBufferCount = 1U
    };

    CheckVulkanResult(vkAllocateCommandBuffers(LogicalDevice, &CommandBufferAllocateInfo, &CommandBuffer));
}

void ThreadResources::Free(VkDevice const &LogicalDevice)
{
    if (CommandBuffer == VK_NULL_HANDLE || CommandPool == VK_NULL_HANDLE)
    {
        return;
    }

    vkFreeCommandBuffers(LogicalDevice, CommandPool, 1U, &CommandBuffer);
    CommandBuffer = VK_NULL_HANDLE;
}

void ThreadResources::Destroy(VkDevice const &LogicalDevice)
{
    if (CommandPool == VK_NULL_HANDLE)
    {
        return;
    }

    Free(LogicalDevice);

    vkDestroyCommandPool(LogicalDevice, CommandPool, nullptr);
    CommandPool = VK_NULL_HANDLE;
}

void ThreadResources::Reset(VkDevice const &LogicalDevice)
{
    if (CommandPool == VK_NULL_HANDLE)
    {
        return;
    }

    CheckVulkanResult(vkResetCommandPool(LogicalDevice, CommandPool, 0U));
}

struct CommandResources
{
    std::unordered_map<std::uint8_t, ThreadResources> MultiThreadResources {};
    VkCommandPool                                     PrimaryCommandPool { VK_NULL_HANDLE };
    VkCommandBuffer                                   PrimaryCommandBuffer { VK_NULL_HANDLE };
};

std::uint32_t                              g_ObjectsPerThread { 0U };
std::uint32_t                              g_NumThreads { 0U };
std::array<CommandResources, g_ImageCount> g_CommandResources {};

void RenderCore::SetNumObjectsPerThread(std::uint32_t const NumObjects)
{
    if (NumObjects > 0U)
    {
        g_ObjectsPerThread = static_cast<std::uint32_t>(std::ceil(static_cast<float>(NumObjects) / static_cast<float>(g_NumThreads)));
    }
    else
    {
        g_ObjectsPerThread = 0U;
    }
}

void RenderCore::ResetCommandPool(std::uint32_t const Index)
{
    g_ThreadPool.Wait();
    VkDevice const &LogicalDevice = GetLogicalDevice();

    std::for_each(std::execution::unseq,
                  std::begin(g_CommandResources.at(Index).MultiThreadResources),
                  std::end(g_CommandResources.at(Index).MultiThreadResources),
                  [&](auto &ThreadResourcesIt)
                  {
                      ThreadResourcesIt.second.Reset(LogicalDevice);
                  });

    if (g_OnCommandPoolResetCallback)
    {
        g_OnCommandPoolResetCallback(static_cast<std::uint8_t>(Index));
    }

    vkResetCommandPool(LogicalDevice, g_CommandResources.at(Index).PrimaryCommandPool, 0U);
}

void RenderCore::FreeCommandBuffers()
{
    g_ThreadPool.Wait();
    VkDevice const &LogicalDevice = GetLogicalDevice();

    std::for_each(std::execution::unseq,
                  std::begin(g_CommandResources),
                  std::end(g_CommandResources),
                  [&](auto &CommandResourceIt)
                  {
                      std::for_each(std::execution::unseq,
                                    std::begin(CommandResourceIt.MultiThreadResources),
                                    std::end(CommandResourceIt.MultiThreadResources),
                                    [&](auto &ThreadResourcesIt)
                                    {
                                        ThreadResourcesIt.second.Free(LogicalDevice);
                                    });

                      vkFreeCommandBuffers(LogicalDevice, CommandResourceIt.PrimaryCommandPool, 1U, &CommandResourceIt.PrimaryCommandBuffer);
                  });
}

void RenderCore::InitializeCommandsResources(std::uint32_t const QueueFamily)
{
    g_NumThreads = static_cast<std::uint8_t>(std::thread::hardware_concurrency());
    g_ThreadPool.SetupCPUThreads("RenderThread");

    VkDevice const &LogicalDevice = GetLogicalDevice();

    std::for_each(std::execution::unseq,
                  std::begin(g_CommandResources),
                  std::end(g_CommandResources),
                  [&](auto &CommandResourceIt)
                  {
                      for (std::uint8_t ThreadIndex = 0U; ThreadIndex < g_NumThreads; ++ThreadIndex)
                      {
                          ThreadResources NewResource {};
                          NewResource.Allocate(LogicalDevice, QueueFamily);
                          CommandResourceIt.MultiThreadResources.emplace(ThreadIndex, std::move(NewResource));
                      }

                      CommandResourceIt.PrimaryCommandPool = CreateCommandPool(QueueFamily, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

                      VkCommandBufferAllocateInfo const CommandBufferAllocateInfo {
                              .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                              .commandPool = CommandResourceIt.PrimaryCommandPool,
                              .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                              .commandBufferCount = 1U
                      };

                      CheckVulkanResult(vkAllocateCommandBuffers(LogicalDevice, &CommandBufferAllocateInfo, &CommandResourceIt.PrimaryCommandBuffer));
                  });
}

void RenderCore::ReleaseCommandsResources()
{
    g_ThreadPool.Wait();

    VkDevice const &LogicalDevice = GetLogicalDevice();

    std::for_each(std::execution::unseq,
                  std::begin(g_CommandResources),
                  std::end(g_CommandResources),
                  [&](auto &CommandResourceIt)
                  {
                      std::for_each(std::execution::unseq,
                                    std::begin(CommandResourceIt.MultiThreadResources),
                                    std::end(CommandResourceIt.MultiThreadResources),
                                    [&](auto &ThreadResourcesIt)
                                    {
                                        ThreadResourcesIt.second.Reset(LogicalDevice);
                                        ThreadResourcesIt.second.Free(LogicalDevice);
                                        ThreadResourcesIt.second.Destroy(LogicalDevice);
                                    });

                      CheckVulkanResult(vkResetCommandPool(LogicalDevice, CommandResourceIt.PrimaryCommandPool, 0U));
                      vkFreeCommandBuffers(LogicalDevice, CommandResourceIt.PrimaryCommandPool, 1U, &CommandResourceIt.PrimaryCommandBuffer);
                      vkDestroyCommandPool(LogicalDevice, CommandResourceIt.PrimaryCommandPool, nullptr);
                      CommandResourceIt.PrimaryCommandPool   = VK_NULL_HANDLE;
                      CommandResourceIt.PrimaryCommandBuffer = VK_NULL_HANDLE;
                  });
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

void BeginRendering(VkCommandBuffer const &CommandBuffer,
                    ImageAllocation const &SwapchainAllocation,
                    ImageAllocation const &DepthAllocation,
                    ImageAllocation const &OffscreenAllocation)
{
    std::vector ImageBarriers
    {
            RenderCore::MountImageBarrier<g_UndefinedLayout,
                                          g_AttachmentLayout,
                                          g_ImageAspect>(SwapchainAllocation.Image,
                                                         SwapchainAllocation.Format),
            RenderCore::MountImageBarrier<g_UndefinedLayout,
                                          g_AttachmentLayout,
                                          g_DepthAspect>(DepthAllocation.Image,
                                                        DepthAllocation.Format)
    };

    bool const &HasOffscreenRendering = Renderer::GetRenderOffscreen();

    if (HasOffscreenRendering && OffscreenAllocation.Image != VK_NULL_HANDLE)
    {
        ImageBarriers.push_back(RenderCore::MountImageBarrier<g_UndefinedLayout,
                                                              g_AttachmentLayout,
                                                              g_ImageAspect>(OffscreenAllocation.Image,
                                                                             OffscreenAllocation.Format));
    }

    VkDependencyInfo const DependencyInfo {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = static_cast<std::uint32_t>(std::size(ImageBarriers)),
            .pImageMemoryBarriers = std::data(ImageBarriers)
    };

    vkCmdPipelineBarrier2(CommandBuffer, &DependencyInfo);

    VkRenderingAttachmentInfo const ColorAttachment {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = HasOffscreenRendering ? OffscreenAllocation.View : SwapchainAllocation.View,
            .imageLayout = g_AttachmentLayout,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = g_ClearValues.at(0U)
    };

    VkRenderingAttachmentInfo const DepthAttachmentInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = DepthAllocation.View,
            .imageLayout = g_AttachmentLayout,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = g_ClearValues.at(1U)
    };

    VkRenderingInfo const RenderingInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT,
            .renderArea = { .offset = { 0, 0 }, .extent = SwapchainAllocation.Extent },
            .layerCount = 1U,
            .colorAttachmentCount = 1U,
            .pColorAttachments = &ColorAttachment,
            .pDepthAttachment = &DepthAttachmentInfo,
            .pStencilAttachment = &DepthAttachmentInfo
    };

    vkCmdBeginRendering(CommandBuffer, &RenderingInfo);
}

void EndRendering(VkCommandBuffer const &CommandBuffer, ImageAllocation const &SwapchainAllocation, ImageAllocation const &OffscreenAllocation)
{
    vkCmdEndRendering(CommandBuffer);

    if (Renderer::GetRenderOffscreen() && OffscreenAllocation.Image != VK_NULL_HANDLE)
    {
        RenderCore::RequestImageLayoutTransition<g_AttachmentLayout, g_ReadLayout, g_ImageAspect>(CommandBuffer,
                                                                                                  OffscreenAllocation.Image,
                                                                                                  OffscreenAllocation.Format);
    }

    if (g_OnCommandBufferRecordCallback)
    {
        g_OnCommandBufferRecordCallback(CommandBuffer, SwapchainAllocation);
    }

    RenderCore::RequestImageLayoutTransition<g_AttachmentLayout, g_PresentLayout, g_ImageAspect>(CommandBuffer,
                                                                                                 SwapchainAllocation.Image,
                                                                                                 SwapchainAllocation.Format);
}

std::vector<VkCommandBuffer> RecordSceneCommands(std::uint32_t const    ImageIndex,
                                                 ImageAllocation const &SwapchainAllocation,
                                                 ImageAllocation const &DepthAllocation)
{
    VkCommandBufferInheritanceRenderingInfo const InheritanceRenderingInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO,
            .flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT,
            .colorAttachmentCount = 1U,
            .pColorAttachmentFormats = &SwapchainAllocation.Format,
            .depthAttachmentFormat = DepthAllocation.Format,
            .stencilAttachmentFormat = DepthAllocation.Format,
            .rasterizationSamples = g_MSAASamples,
    };

    VkCommandBufferInheritanceInfo const InheritanceInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
            .pNext = &InheritanceRenderingInfo
    };

    VkCommandBufferBeginInfo SecondaryBeginInfo = g_CommandBufferBeginInfo;
    SecondaryBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    SecondaryBeginInfo.pInheritanceInfo = &InheritanceInfo;

    auto const &Objects = GetObjects();
    if (Objects.empty())
    {
        return {};
    }

    VkPipeline const &      Pipeline       = GetMainPipeline();
    VkPipelineLayout const &PipelineLayout = GetPipelineLayout();
    Camera const &          Camera         = GetCamera();

    std::vector<VkCommandBuffer> Output;
    Output.reserve(g_NumThreads);

    CommandResources const &CommandResources = g_CommandResources.at(ImageIndex);

    std::vector<std::uint32_t> ThreadIndices(g_NumThreads);
    std::iota(std::begin(ThreadIndices), std::end(ThreadIndices), 0U);

    auto ProcessCommandBuffer = [&](std::uint32_t const ThreadIndex)
    {
        auto const &[CommandPool, CommandBuffer] = CommandResources.MultiThreadResources.at(ThreadIndex);

        if (CommandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        CheckVulkanResult(vkBeginCommandBuffer(CommandBuffer, &SecondaryBeginInfo));
        SetViewport(CommandBuffer, SwapchainAllocation.Extent);

        bool HasDraw = false;

        for (std::uint32_t ObjectIndex = 0U; ObjectIndex < g_ObjectsPerThread; ++ObjectIndex)
        {
            std::uint32_t const ObjectAccessIndex = ThreadIndex * g_ObjectsPerThread + ObjectIndex;

            if (ObjectAccessIndex >= Objects.size())
            {
                break;
            }

            if (auto const &Object = Objects.at(ObjectAccessIndex);
                Camera.CanDrawObject(Object))
            {
                if (!HasDraw)
                {
                    HasDraw = true;
                    vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
                }

                Object->UpdateUniformBuffers();
                Object->DrawObject(CommandBuffer, PipelineLayout, ObjectAccessIndex);
            }
        }

        CheckVulkanResult(vkEndCommandBuffer(CommandBuffer));
    };

    std::for_each(std::execution::unseq,
                  std::cbegin(ThreadIndices),
                  std::cend(ThreadIndices),
                  [&](std::uint32_t const ThreadIndex)
                  {
                      g_ThreadPool.AddTask([ProcessCommandBuffer, ThreadIndex]
                                           {
                                               ProcessCommandBuffer(ThreadIndex);
                                           },
                                           ThreadIndex);
                  });

    std::for_each(std::execution::unseq,
                  std::cbegin(CommandResources.MultiThreadResources),
                  std::cend(CommandResources.MultiThreadResources),
                  [&](auto const &ResourcePair)
                  {
                      if (ResourcePair.second.CommandBuffer != VK_NULL_HANDLE)
                      {
                          Output.push_back(ResourcePair.second.CommandBuffer);
                      }
                  });

    g_ThreadPool.Wait();

    return Output;
}

void RenderCore::RecordCommandBuffers(std::uint32_t const ImageIndex)
{
    ImageAllocation const &SwapchainAllocation = GetSwapChainImages().at(ImageIndex);
    ImageAllocation const &DepthAllocation     = GetDepthImage();
    ImageAllocation const &OffscreenAllocation = GetOffscreenImages().at(ImageIndex);

    VkCommandBuffer const &CommandBuffer = g_CommandResources.at(ImageIndex).PrimaryCommandBuffer;
    CheckVulkanResult(vkBeginCommandBuffer(CommandBuffer, &g_CommandBufferBeginInfo));

    BeginRendering(CommandBuffer, SwapchainAllocation, DepthAllocation, OffscreenAllocation);

    if (std::vector<VkCommandBuffer> const CommandBuffers = RecordSceneCommands(ImageIndex, SwapchainAllocation, DepthAllocation);
        !std::empty(CommandBuffers))
    {
        vkCmdExecuteCommands(CommandBuffer, static_cast<std::uint32_t>(std::size(CommandBuffers)), std::data(CommandBuffers));
    }

    EndRendering(CommandBuffer, SwapchainAllocation, OffscreenAllocation);
    CheckVulkanResult(vkEndCommandBuffer(CommandBuffer));
}

void RenderCore::SubmitCommandBuffers(std::uint32_t const ImageIndex)
{
    VkSemaphoreSubmitInfo const WaitSemaphoreInfo {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = GetImageAvailableSemaphore(ImageIndex),
            .value = 1U,
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSemaphoreSubmitInfo const SignalSemaphoreInfo {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = GetRenderFinishedSemaphore(ImageIndex),
            .value = 1U,
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkCommandBuffer const &         CommandBuffer = g_CommandResources.at(ImageIndex).PrimaryCommandBuffer;
    VkCommandBufferSubmitInfo const PrimarySubmission { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = CommandBuffer };

    VkSubmitInfo2 const SubmitInfo {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .waitSemaphoreInfoCount = 1U,
            .pWaitSemaphoreInfos = &WaitSemaphoreInfo,
            .commandBufferInfoCount = 1U,
            .pCommandBufferInfos = &PrimarySubmission,
            .signalSemaphoreInfoCount = 1U,
            .pSignalSemaphoreInfos = &SignalSemaphoreInfo
    };

    auto const &Queue = GetGraphicsQueue().second;
    CheckVulkanResult(vkQueueSubmit2(Queue, 1U, &SubmitInfo, GetFence(ImageIndex)));
    SetFenceWaitStatus(ImageIndex, true);

    if (Renderer::GetUseDefaultSync())
    {
        WaitAndResetFence(ImageIndex);
    }
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

    VkCommandBufferAllocateInfo const CommandBufferAllocateInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = CommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<std::uint32_t>(std::size(CommandBuffers)),
    };

    VkDevice const &LogicalDevice = GetLogicalDevice();

    CheckVulkanResult(vkAllocateCommandBuffers(LogicalDevice, &CommandBufferAllocateInfo, std::data(CommandBuffers)));

    std::for_each(std::execution::unseq,
                  std::cbegin(CommandBuffers),
                  std::cend(CommandBuffers),
                  [&](VkCommandBuffer const &CommandBufferIter)
                  {
                      CheckVulkanResult(vkBeginCommandBuffer(CommandBufferIter, &g_CommandBufferBeginInfo));
                  });
}

void RenderCore::FinishSingleCommandQueue(VkQueue const &Queue, VkCommandPool const &CommandPool, std::vector<VkCommandBuffer> const &CommandBuffers)
{
    if (std::empty(CommandBuffers) || CommandPool == VK_NULL_HANDLE)
    {
        return;
    }

    std::vector<VkCommandBufferSubmitInfo> CommandBufferInfos;
    CommandBufferInfos.reserve(std::size(CommandBuffers));

    std::for_each(std::execution::unseq,
                  std::cbegin(CommandBuffers),
                  std::cend(CommandBuffers),
                  [&](VkCommandBuffer const &CommandBufferIter)
                  {
                      CheckVulkanResult(vkEndCommandBuffer(CommandBufferIter));

                      CommandBufferInfos.push_back(VkCommandBufferSubmitInfo {
                                                           .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                                                           .commandBuffer = CommandBufferIter,
                                                           .deviceMask = 0U
                                                   });
                  });

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
