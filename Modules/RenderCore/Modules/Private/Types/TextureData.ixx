// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <volk.h>

export module RenderCore.Types.TextureData;

export namespace RenderCore
{
    struct VulkanTextureData
    {
        VkImageView ImageView;
        VkSampler Sampler;
    };
}// namespace RenderCore
