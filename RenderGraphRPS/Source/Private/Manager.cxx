// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <Volk/volk.h>
#include <array>
#include <rps/rps.h>
#include <span>

module RenderGraphRPS.Manager;

import RenderGraphRPS.Utils;

using namespace RenderGraphRPS;

static RpsDevice s_RPSDevice {nullptr};

bool RenderGraphRPS::CreateRPSDevice(VkDevice const &LogicalDevice, VkPhysicalDevice const &PhysicalDevice)
{
    RpsVKFunctions RPSVKFunctions {};
#define RPS_VK_FILL_VK_FUNCTION(CallName) RPSVKFunctions.CallName = CallName;
    RPS_VK_FUNCTION_TABLE(RPS_VK_FILL_VK_FUNCTION)

    RpsVKRuntimeDeviceCreateInfo const RPSDeviceCreateInfo {
        .hVkDevice         = LogicalDevice,
        .hVkPhysicalDevice = PhysicalDevice,
        .flags             = RPS_VK_RUNTIME_FLAG_NONE,
        .pVkFunctions      = &RPSVKFunctions,
    };

    return CheckRPSResult(rpsVKRuntimeDeviceCreate(&RPSDeviceCreateInfo, &s_RPSDevice));
}

void RenderGraphRPS::DestroyDevice()
{
    rpsDeviceDestroy(s_RPSDevice);
    s_RPSDevice = nullptr;
}

RpsRenderGraph RenderGraphRPS::CreateRPSRenderGraph(RpsRpslEntry const &EntryPoint)
{
    constexpr std::array<RpsQueueFlags, 3U> QueueFlags = {RPS_QUEUE_FLAG_GRAPHICS, RPS_QUEUE_FLAG_COMPUTE, RPS_QUEUE_FLAG_COPY};

    RpsRenderGraphCreateInfo const RPSRenderGraphCreateInfo {
        .scheduleInfo
        = {.scheduleFlags = RPS_SCHEDULE_UNSPECIFIED, .numQueues = static_cast<std::uint32_t>(std::size(QueueFlags)), .pQueueInfos = std::data(QueueFlags)},
        .mainEntryCreateInfo = {
            .pSignatureDesc  = nullptr,
            .hRpslEntryPoint = EntryPoint,
        }};

    RpsRenderGraph Output;
    CheckRPSResult(rpsRenderGraphCreate(s_RPSDevice, &RPSRenderGraphCreateInfo, &Output));

    return Output;
}

void RenderGraphRPS::DestroyRenderGraph(RpsRenderGraph const &RenderGraph)
{
    rpsRenderGraphDestroy(RenderGraph);
}

void RenderGraphRPS::RecordRenderGraphCommands(RpsRenderGraph const &RenderGraph, VkCommandBuffer const &CommandBuffer)
{
    RpsRenderGraphBatchLayout BatchLayout;
    CheckRPSResult(rpsRenderGraphGetBatchLayout(RenderGraph, &BatchLayout));

    for (std::span const CommandBatchSpan {BatchLayout.pCmdBatches, BatchLayout.numCmdBatches}; RpsCommandBatch const &CommandBatch : CommandBatchSpan)
    {
        RpsRenderGraphRecordCommandInfo RecordInfo {.hCmdBuffer    = rpsVKCommandBufferToHandle(CommandBuffer),
                                                    .cmdBeginIndex = CommandBatch.cmdBegin,
                                                    .numCmds       = CommandBatch.numCmds};

        CheckRPSResult(rpsRenderGraphRecordCommands(RenderGraph, &RecordInfo));
    }
}
