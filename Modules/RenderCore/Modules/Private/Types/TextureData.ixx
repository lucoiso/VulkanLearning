// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <volk.h>

export module RenderCore.Types.TextureData;

export import <cstdint>;

namespace RenderCore
{
    export struct TextureData
    {
        std::uint32_t const ObjectID {0U};
        VkImageView ImageView {VK_NULL_HANDLE};
        VkSampler Sampler {VK_NULL_HANDLE};
    };
}// namespace RenderCore
