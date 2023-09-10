// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include <vulkan/vulkan_core.h>

namespace RenderCore
{
    struct DeviceProperties
    {
        VkSurfaceFormatKHR PreferredFormat;
        VkPresentModeKHR PreferredMode;
        VkExtent2D PreferredExtent;
        VkSurfaceCapabilitiesKHR Capabilities;

        inline bool IsValid() const { return PreferredExtent.height != 0u && PreferredExtent.width != 0u; }
    };
}