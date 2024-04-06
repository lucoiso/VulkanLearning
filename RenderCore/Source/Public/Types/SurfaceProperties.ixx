// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <Volk/volk.h>

export module RenderCore.Types.SurfaceProperties;

import RenderCore.Types.Transform;

namespace RenderCore
{
    export struct SurfaceProperties
    {
        VkSurfaceFormatKHR Format{};
        VkFormat           DepthFormat{};
        VkPresentModeKHR   Mode{};
        VkExtent2D         Extent{};

        [[nodiscard]] bool IsValid() const
        {
            return Format.format != VK_FORMAT_UNDEFINED
                && Format.colorSpace != VK_COLOR_SPACE_MAX_ENUM_KHR
                && DepthFormat != VK_FORMAT_UNDEFINED
                && Mode != VK_PRESENT_MODE_MAX_ENUM_KHR
                && Extent.width != 0U
                && Extent.height != 0U;
        }

        bool operator!=(SurfaceProperties const &Other) const
        {
            return Format.format != Other.Format.format
                || Format.colorSpace != Other.Format.colorSpace
                || DepthFormat != Other.DepthFormat
                || Mode != Other.Mode
                || Extent.width != Other.Extent.width;
        }
    };
} // namespace RenderCore
