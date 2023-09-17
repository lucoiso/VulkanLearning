// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "Types/TextureData.h"
#include <vulkan/vulkan.h>

namespace RenderCore
{
    struct VulkanTextureData
    {
        VkImageView ImageView;
        VkSampler Sampler;

        VulkanTextureData() = default;
        VulkanTextureData(VkImageView imageView, VkSampler sampler)
            : ImageView(imageView)
            , Sampler(sampler) {}
    };
}