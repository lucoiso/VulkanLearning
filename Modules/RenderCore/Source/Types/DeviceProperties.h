// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include <vulkan/vulkan_core.h>

namespace RenderCore
{
    struct VulkanDeviceProperties
    {
        VkSurfaceFormatKHR Format;
        VkFormat DepthFormat;
        VkPresentModeKHR Mode;
        VkExtent2D Extent;
        VkSurfaceCapabilitiesKHR Capabilities;

        inline bool IsValid() const { return Extent.height != 0u && Extent.width != 0u; }
    };
}