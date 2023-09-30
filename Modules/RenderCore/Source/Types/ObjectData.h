// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

#pragma once

#include "Types/TextureData.h"
#include <volk.h>
#include <vector>

namespace RenderCore
{
    struct VulkanObjectData
    {
        std::vector<VkBuffer> UniformBuffers;
        std::vector<VulkanTextureData> TextureDatas;
    };
}
