// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

#pragma once

#include "Types/TextureData.h"
#include <vector>
#include <volk.h>

namespace RenderCore
{
    struct VulkanObjectData
    {
        std::vector<VkBuffer> UniformBuffers;
        std::vector<VulkanTextureData> TextureDatas;
    };
}// namespace RenderCore
