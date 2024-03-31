// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <Volk/volk.h>
#include <array>
#include <boost/log/trivial.hpp>
#include <optional>
#include "Utils/Library/Macros.h"

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
    #include <imgui.h>
    #include <imgui_impl_vulkan.h>
#endif

module RenderCore.Runtime.Command;

import RuntimeInfo.Manager;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import RenderCore.Utils.DebugHelpers;
import RenderCore.Utils.EnumConverter;
import RenderCore.Runtime.Device;
import RenderCore.Runtime.Pipeline;
import RenderCore.Integrations.ImGuiOverlay;
import RenderCore.Runtime.Buffer;
import RenderCore.Runtime.Buffer.Operations;

using namespace RenderCore;

VkCommandPool                g_CommandPool {};
std::vector<VkCommandBuffer> g_CommandBuffers {};
VkSemaphore                  g_ImageAvailableSemaphore {};
VkSemaphore                  g_RenderFinishedSemaphore {};
VkFence                      g_Fence {};

void AllocateCommandBuffers(std::uint32_t const QueueFamily, std::uint8_t const NumberOfBuffers)
{
    PUSH_CALLSTACK();

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

    VkCommandBufferAllocateInfo const CommandBufferAllocateInfo {.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                                                 .commandPool        = g_CommandPool,
                                                                 .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                                 .commandBufferCount = static_cast<std::uint32_t>(std::size(g_CommandBuffers))};

    CheckVulkanResult(vkAllocateCommandBuffers(volkGetLoadedDevice(), &CommandBufferAllocateInfo, std::data(g_CommandBuffers)));
}

void WaitAndResetFences()
{
    PUSH_CALLSTACK();

    if (g_Fence == VK_NULL_HANDLE)
    {
        return;
    }

    CheckVulkanResult(vkWaitForFences(volkGetLoadedDevice(), 1U, &g_Fence, VK_TRUE, g_Timeout));
    CheckVulkanResult(vkResetFences(volkGetLoadedDevice(), 1U, &g_Fence));
}

void FreeCommandBuffers()
{
    PUSH_CALLSTACK();

    if (!std::empty(g_CommandBuffers))
    {
        vkFreeCommandBuffers(volkGetLoadedDevice(), g_CommandPool, static_cast<std::uint32_t>(std::size(g_CommandBuffers)), std::data(g_CommandBuffers));

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
    PUSH_CALLSTACK_WITH_COUNTER();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Releasing vulkan commands resources";

    DestroyCommandsSynchronizationObjects();
}

VkCommandPool RenderCore::CreateCommandPool(std::uint8_t const FamilyQueueIndex)
{
    PUSH_CALLSTACK();

    VkCommandPoolCreateInfo const CommandPoolCreateInfo {.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                                         .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                                                         .queueFamilyIndex = static_cast<std::uint32_t>(FamilyQueueIndex)};

    VkCommandPool Output = VK_NULL_HANDLE;
    CheckVulkanResult(vkCreateCommandPool(volkGetLoadedDevice(), &CommandPoolCreateInfo, nullptr, &Output));

    return Output;
}

void RenderCore::CreateCommandsSynchronizationObjects()
{
    PUSH_CALLSTACK_WITH_COUNTER();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating vulkan synchronization objects";

    constexpr VkSemaphoreCreateInfo SemaphoreCreateInfo {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    constexpr VkFenceCreateInfo FenceCreateInfo {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT};

    CheckVulkanResult(vkCreateSemaphore(volkGetLoadedDevice(), &SemaphoreCreateInfo, nullptr, &g_ImageAvailableSemaphore));
    CheckVulkanResult(vkCreateSemaphore(volkGetLoadedDevice(), &SemaphoreCreateInfo, nullptr, &g_RenderFinishedSemaphore));

    CheckVulkanResult(vkCreateFence(volkGetLoadedDevice(), &FenceCreateInfo, nullptr, &g_Fence));
    CheckVulkanResult(vkResetFences(volkGetLoadedDevice(), 1U, &g_Fence));
}

void RenderCore::DestroyCommandsSynchronizationObjects()
{
    PUSH_CALLSTACK_WITH_COUNTER();
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Destroying vulkan synchronization objects";

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

std::optional<std::int32_t> RenderCore::RequestSwapChainImage(VkSwapchainKHR const &SwapChain)
{
    PUSH_CALLSTACK();

    if (g_ImageAvailableSemaphore == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan semaphore: ImageAllocation Available is invalid.");
    }

    if (g_Fence == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan fence is invalid.");
    }

    std::uint32_t  Output          = 0U;
    VkResult const OperationResult = vkAcquireNextImageKHR(volkGetLoadedDevice(), SwapChain, g_Timeout, g_ImageAvailableSemaphore, g_Fence, &Output);
    WaitAndResetFences();

    if (OperationResult != VK_SUCCESS)
    {
        if (OperationResult == VK_SUBOPTIMAL_KHR)
        {
            return std::nullopt;
        }

        throw std::runtime_error("Failed to acquire Vulkan swap chain image.");
    }

    return std::optional {static_cast<std::int32_t>(Output)};
}

constexpr std::array<VkDeviceSize, 1U> Offsets {0U};

constexpr VkImageAspectFlags ImageAspect = VK_IMAGE_ASPECT_COLOR_BIT;
constexpr VkImageAspectFlags DepthAspect = VK_IMAGE_ASPECT_DEPTH_BIT;

constexpr VkImageLayout UndefinedLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
constexpr VkImageLayout SwapChainMidLayout   = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
constexpr VkImageLayout SwapChainFinalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
constexpr VkImageLayout DepthLayout          = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;

constexpr VkCommandBufferBeginInfo CommandBufferBeginInfo {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                                           .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

void BindDescriptorSets(VkCommandBuffer const                      &CommandBuffer,
                        VkPipelineLayout const                     &PipelineLayout,
                        Camera const                               &Camera,
                        std::vector<std::shared_ptr<Object>> const &Objects,
                        VkExtent2D const                           &SwapChainExtent)
{
    PUSH_CALLSTACK();

    auto const &SceneBuffer = GetSceneUniformDescriptor();

    auto const &Allocations = GetAllocatedObjects();

    for (std::shared_ptr<Object> const &ObjectIter : Objects)
    {
        if (!ObjectIter || ObjectIter->IsPendingDestroy() || !Camera.CanDrawObject(ObjectIter, SwapChainExtent))
        {
            continue;
        }

        std::uint32_t const ObjectID = ObjectIter->GetID();

        if (!ContainsObject(ObjectID))
        {
            continue;
        }

        const auto &[Object, Allocation, Vertices, Indices, ImageCreationDatas, CommandBufferSets] = Allocations.at(ObjectID);

        std::vector WriteDescriptors {VkWriteDescriptorSet {
                                          .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                          .dstSet          = VK_NULL_HANDLE,
                                          .dstBinding      = 0U,
                                          .dstArrayElement = 0U,
                                          .descriptorCount = 1U,
                                          .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                          .pBufferInfo     = &SceneBuffer,
                                      },
                                      VkWriteDescriptorSet {
                                          .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                          .dstSet          = VK_NULL_HANDLE,
                                          .dstBinding      = 1U,
                                          .dstArrayElement = 0U,
                                          .descriptorCount = static_cast<std::uint32_t>(std::size(Allocation.ModelDescriptors)),
                                          .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                          .pBufferInfo     = std::data(Allocation.ModelDescriptors),
                                      }};

        for (auto &TextureDescriptorIter : Allocation.TextureDescriptors)
        {
            WriteDescriptors.push_back(VkWriteDescriptorSet {
                .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet          = VK_NULL_HANDLE,
                .dstBinding      = 2U + static_cast<std::uint32_t>(TextureDescriptorIter.first),
                .dstArrayElement = 0U,
                .descriptorCount = 1U,
                .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo      = &TextureDescriptorIter.second,
            });
        }

        vkCmdPushDescriptorSetKHR(CommandBuffer,
                                  VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  PipelineLayout,
                                  0U,
                                  static_cast<std::uint32_t>(std::size(WriteDescriptors)),
                                  std::data(WriteDescriptors));

        VkBuffer const     &VertexBuffer = GetVertexBuffer(ObjectID);
        VkBuffer const     &IndexBuffer  = GetIndexBuffer(ObjectID);
        std::uint32_t const IndexCount   = GetIndicesCount(ObjectID);

        UpdateModelUniformBuffers(ObjectIter);

        bool ActiveVertexBinding = false;
        if (VertexBuffer != VK_NULL_HANDLE)
        {
            vkCmdBindVertexBuffers(CommandBuffer, 0U, 1U, &VertexBuffer, std::data(Offsets));
            ActiveVertexBinding = true;
        }

        bool ActiveIndexBinding = false;
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
}

void RecordSceneCommands(VkCommandBuffer                            &CommandBuffer,
                         std::uint32_t const                         ImageIndex,
                         Camera const                               &Camera,
                         std::vector<std::shared_ptr<Object>> const &Objects,
                         VkExtent2D const                           &SwapChainExtent)
{
    PUSH_CALLSTACK();
    CheckVulkanResult(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));

    {
        VkViewport const Viewport {.x        = 0.F,
                                   .y        = 0.F,
                                   .width    = static_cast<float>(SwapChainExtent.width),
                                   .height   = static_cast<float>(SwapChainExtent.height),
                                   .minDepth = 0.F,
                                   .maxDepth = 1.F};

        VkRect2D const Scissor {.offset = {0, 0}, .extent = SwapChainExtent};

        vkCmdSetViewport(CommandBuffer, 0U, 1U, &Viewport);
        vkCmdSetScissor(CommandBuffer, 0U, 1U, &Scissor);
    }

    VkFormat const SwapChainFormat = GetSwapChainImageFormat();

    std::vector<VkRenderingAttachmentInfoKHR> ColorAttachments {};
    ColorAttachments.reserve(2U);

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
    constexpr VkImageLayout ViewportMidLayout   = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    constexpr VkImageLayout ViewportFinalLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;

    auto const &[ViewportImage, ViewportView, ViewportAllocation, ViewportType] = GetViewportImages().at(ImageIndex);
    RenderCore::MoveImageLayout<UndefinedLayout, ViewportMidLayout, ImageAspect>(CommandBuffer, ViewportImage, SwapChainFormat);

    ColorAttachments.push_back(VkRenderingAttachmentInfoKHR {.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                                                             .imageView   = ViewportView,
                                                             .imageLayout = ViewportMidLayout,
                                                             .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                             .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
                                                             .clearValue  = g_ClearValues.at(0U)});
#endif

    auto const &[SwapChainImage, SwapChainView, SwapChainAllocation, SwapChainType] = GetSwapChainImages().at(ImageIndex);
    RenderCore::MoveImageLayout<UndefinedLayout, SwapChainMidLayout, ImageAspect>(CommandBuffer, SwapChainImage, SwapChainFormat);

    ColorAttachments.push_back(VkRenderingAttachmentInfoKHR {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
#ifdef VULKAN_RENDERER_ENABLE_IMGUI
        .imageView   = ViewportView,
        .imageLayout = ViewportMidLayout,
        .loadOp      = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE,
#else
        .imageView   = SwapChainView,
        .imageLayout = SwapChainMidLayout,
        .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue  = g_ClearValues.at(0U)
#endif
    });

    ColorAttachments.shrink_to_fit();

    auto const &[DepthImage, DepthView, DepthAllocation, DepthType] = GetDepthImage();
    VkFormat const DepthFormat                                      = GetDepthFormat();
    RenderCore::MoveImageLayout<UndefinedLayout, DepthLayout, DepthAspect>(CommandBuffer, DepthImage, DepthFormat);

    VkRenderingAttachmentInfoKHR const DepthAttachmentInfo {.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                                                            .imageView   = DepthView,
                                                            .imageLayout = DepthLayout,
                                                            .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                            .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
                                                            .clearValue  = g_ClearValues.at(1U)};

    VkRenderingInfo const RenderingInfo {.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
                                         .renderArea           = {.offset = {0, 0}, .extent = SwapChainExtent},
                                         .layerCount           = 1U,
                                         .colorAttachmentCount = static_cast<std::uint32_t>(std::size(ColorAttachments)),
                                         .pColorAttachments    = std::data(ColorAttachments),
                                         .pDepthAttachment     = &DepthAttachmentInfo,
                                         .pStencilAttachment   = &DepthAttachmentInfo};

    vkCmdBeginRendering(CommandBuffer, &RenderingInfo);
    {
        VkPipeline const        Pipeline       = GetMainPipeline();
        VkPipelineLayout const &PipelineLayout = GetPipelineLayout();

        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
        BindDescriptorSets(CommandBuffer, PipelineLayout, Camera, Objects, SwapChainExtent);
    }
    vkCmdEndRendering(CommandBuffer);

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
    RenderCore::MoveImageLayout<ViewportMidLayout, ViewportFinalLayout, ImageAspect>(CommandBuffer, ViewportImage, SwapChainFormat);

    if (!IsImGuiInitialized())
    {
        RenderCore::MoveImageLayout<SwapChainMidLayout, SwapChainFinalLayout, ImageAspect>(CommandBuffer, SwapChainImage, SwapChainFormat);
    }
#else
    RenderCore::MoveImageLayout<SwapChainMidLayout, SwapChainFinalLayout, ImageAspect>(CommandBuffer, SwapChainImage, SwapChainFormat);
#endif

    CheckVulkanResult(vkEndCommandBuffer(CommandBuffer));
}

void RenderCore::RecordCommandBuffers(std::uint32_t const                         ImageIndex,
                                      Camera const                               &Camera,
                                      std::vector<std::shared_ptr<Object>> const &Objects,
                                      VkExtent2D const                           &SwapChainExtent)
{
    PUSH_CALLSTACK();

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
    AllocateCommandBuffers(GetGraphicsQueue().first, 1U + static_cast<std::uint8_t>(IsImGuiInitialized()));
#else
    AllocateCommandBuffer(GetGraphicsQueue().first, 1U);
#endif

    RecordSceneCommands(g_CommandBuffers.at(0U), ImageIndex, Camera, Objects, SwapChainExtent);

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
    if (IsImGuiInitialized())
    {
        VkCommandBuffer &CommandBuffer = g_CommandBuffers.at(1U);
        CheckVulkanResult(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));

        auto const &[SwapChainImage, SwapChainView, SwapChainAllocation, SwapChainType] = GetSwapChainImages().at(ImageIndex);
        VkFormat const SwapChainFormat                                                  = GetSwapChainImageFormat();

        if (ImDrawData *const ImGuiDrawData = ImGui::GetDrawData())
        {
            VkRenderingAttachmentInfoKHR const ColorAttachmentInfo {.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                                                                    .imageView   = SwapChainView,
                                                                    .imageLayout = SwapChainMidLayout,
                                                                    .loadOp      = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                                                    .storeOp     = VK_ATTACHMENT_STORE_OP_STORE};

            VkRenderingInfo const RenderingInfo {.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
                                                 .renderArea           = {.offset = {0, 0}, .extent = SwapChainExtent},
                                                 .layerCount           = 1U,
                                                 .colorAttachmentCount = 1U,
                                                 .pColorAttachments    = &ColorAttachmentInfo};

            vkCmdBeginRendering(CommandBuffer, &RenderingInfo);
            ImGui_ImplVulkan_RenderDrawData(ImGuiDrawData, CommandBuffer);
            vkCmdEndRendering(CommandBuffer);
        }

        RenderCore::MoveImageLayout<SwapChainMidLayout, SwapChainFinalLayout, ImageAspect>(CommandBuffer, SwapChainImage, SwapChainFormat);

        CheckVulkanResult(vkEndCommandBuffer(CommandBuffer));
    }
#endif
}

void RenderCore::SubmitCommandBuffers()
{
    PUSH_CALLSTACK();

    VkSemaphoreSubmitInfoKHR WaitSemaphoreInfo = {.sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR,
                                                  .semaphore   = g_ImageAvailableSemaphore,
                                                  .value       = 1U,
                                                  .stageMask   = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
                                                  .deviceIndex = 0U};

    VkSemaphoreSubmitInfoKHR SignalSemaphoreInfo = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR, .semaphore = g_RenderFinishedSemaphore, .value = 1U};

    std::vector<VkCommandBufferSubmitInfoKHR> CommandBufferInfos;
    CommandBufferInfos.reserve(std::size(g_CommandBuffers));

    for (VkCommandBuffer const &CommandBufferIter : g_CommandBuffers)
    {
        CommandBufferInfos.push_back(
            VkCommandBufferSubmitInfo {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR, .commandBuffer = CommandBufferIter, .deviceMask = 0U});
    }

    VkSubmitInfo2KHR const SubmitInfo = {.sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR,
                                         .waitSemaphoreInfoCount   = 1U,
                                         .pWaitSemaphoreInfos      = &WaitSemaphoreInfo,
                                         .commandBufferInfoCount   = static_cast<std::uint32_t>(std::size(CommandBufferInfos)),
                                         .pCommandBufferInfos      = std::data(CommandBufferInfos),
                                         .signalSemaphoreInfoCount = 1U,
                                         .pSignalSemaphoreInfos    = &SignalSemaphoreInfo};

    auto const &GraphicsQueue = GetGraphicsQueue().second;

    CheckVulkanResult(vkQueueSubmit2(GraphicsQueue, 1U, &SubmitInfo, g_Fence));
    WaitAndResetFences();
    FreeCommandBuffers();
}

void RenderCore::PresentFrame(std::uint32_t const ImageIndice, VkSwapchainKHR const &SwapChain)
{
    PUSH_CALLSTACK();

    VkPresentInfoKHR const PresentInfo {.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                                        .waitSemaphoreCount = 1U,
                                        .pWaitSemaphores    = &g_RenderFinishedSemaphore,
                                        .swapchainCount     = 1U,
                                        .pSwapchains        = &SwapChain,
                                        .pImageIndices      = &ImageIndice,
                                        .pResults           = nullptr};

    auto const &Queue = GetGraphicsQueue().second;

    if (VkResult const OperationResult = vkQueuePresentKHR(Queue, &PresentInfo); OperationResult != VK_SUCCESS)
    {
        if (OperationResult != VK_ERROR_OUT_OF_DATE_KHR && OperationResult != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Vulkan operation failed with result: " + std::string(ResultToString(OperationResult)));
        }
    }
}

void RenderCore::InitializeSingleCommandQueue(VkCommandPool &CommandPool, std::vector<VkCommandBuffer> &CommandBuffers, std::uint8_t const QueueFamilyIndex)
{
    PUSH_CALLSTACK();

    VkCommandPoolCreateInfo const CommandPoolCreateInfo {.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
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
    for (VkCommandBuffer const &CommandBufferIter : CommandBuffers)
    {
        CheckVulkanResult(vkBeginCommandBuffer(CommandBufferIter, &CommandBufferBeginInfo));
    }
}

void RenderCore::FinishSingleCommandQueue(VkQueue const &Queue, VkCommandPool const &CommandPool, std::vector<VkCommandBuffer> &CommandBuffers)
{
    PUSH_CALLSTACK();

    if (CommandPool == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan command pool is invalid.");
    }

    std::erase_if(CommandBuffers,
                  [](VkCommandBuffer const &CommandBufferIter)
                  {
                      return CommandBufferIter == VK_NULL_HANDLE;
                  });

    if (std::empty(CommandBuffers))
    {
        throw std::runtime_error("Vulkan command buffer is invalid.");
    }

    std::vector<VkCommandBufferSubmitInfoKHR> CommandBufferInfos;
    CommandBufferInfos.reserve(std::size(g_CommandBuffers));

    for (VkCommandBuffer const &CommandBufferIter : CommandBuffers)
    {
        CheckVulkanResult(vkEndCommandBuffer(CommandBufferIter));

        CommandBufferInfos.push_back(
            VkCommandBufferSubmitInfo {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR, .commandBuffer = CommandBufferIter, .deviceMask = 0U});
    }

    VkSubmitInfo2KHR const SubmitInfo = {.sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR,
                                         .commandBufferInfoCount = static_cast<std::uint32_t>(std::size(CommandBufferInfos)),
                                         .pCommandBufferInfos    = std::data(CommandBufferInfos)};

    CheckVulkanResult(vkQueueSubmit2(Queue, 1U, &SubmitInfo, g_Fence));
    CheckVulkanResult(vkQueueWaitIdle(Queue));

    vkFreeCommandBuffers(volkGetLoadedDevice(), CommandPool, static_cast<std::uint32_t>(std::size(CommandBuffers)), std::data(CommandBuffers));
    vkDestroyCommandPool(volkGetLoadedDevice(), CommandPool, nullptr);
}
