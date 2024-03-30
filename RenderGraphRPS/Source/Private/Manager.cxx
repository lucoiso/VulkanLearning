// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <Volk/volk.h>
#include <rps/rps.h>

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
