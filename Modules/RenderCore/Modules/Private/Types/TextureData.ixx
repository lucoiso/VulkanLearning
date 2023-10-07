// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#pragma once

#include <volk.h>

export module RenderCore.Types.TextureData;

namespace RenderCore
{
    export struct TextureData
    {
        VkImageView ImageView;
        VkSampler Sampler;
    };
}// namespace RenderCore
