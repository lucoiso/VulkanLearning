// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

#pragma once

#include <volk.h>

namespace RenderCore
{
    struct VulkanDeviceProperties
    {
        VkSurfaceFormatKHR Format;
        VkFormat DepthFormat;
        VkPresentModeKHR Mode;
        VkExtent2D Extent;
        VkSurfaceCapabilitiesKHR Capabilities;

        [[nodiscard]] bool IsValid() const
        {
            return Extent.height != 0U && Extent.width != 0U;
        }

        bool operator!=(VulkanDeviceProperties const& Other) const
        {
            return Format.format != Other.Format.format
                   || Format.colorSpace != Other.Format.colorSpace
                   || DepthFormat != Other.DepthFormat
                   || Mode != Other.Mode
                   || Extent.height != Other.Extent.height
                   || Extent.width != Other.Extent.width
                   || Capabilities.currentExtent.height != Other.Capabilities.currentExtent.height
                   || Capabilities.currentExtent.width != Other.Capabilities.currentExtent.width;
        }
    };
}// namespace RenderCore
