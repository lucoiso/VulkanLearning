// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <volk.h>

export module RenderCore.Types.TextureBufferData;

export import <cstdint>;

namespace RenderCore
{
    export enum class TextureType : std::uint8_t {
        Undefined,
        BaseColor
    };

    export struct TextureBufferData
    {
        VkImageView ImageView {VK_NULL_HANDLE};
        VkSampler Sampler {VK_NULL_HANDLE};
    };
}// namespace RenderCore
