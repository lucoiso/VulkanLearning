// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

#pragma once

#include "Types/TextureData.h"
#include <volk.h>

namespace RenderCore
{
    struct VulkanTextureData
    {
        VkImageView ImageView;
        VkSampler Sampler;

        VulkanTextureData() = default;

        VulkanTextureData(const VkImageView InView, const VkSampler InSampler)
            : ImageView(InView)
          , Sampler(InSampler)
        {
        }
    };
}
