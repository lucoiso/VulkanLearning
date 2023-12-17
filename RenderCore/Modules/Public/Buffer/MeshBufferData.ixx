// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <cstdint>
#include <unordered_map>
#include <volk.h>

export module RenderCore.Types.MeshBufferData;

import RenderCore.Types.TextureBufferData;

namespace RenderCore
{
    export struct MeshBufferData
    {
        std::uint32_t ID {0U};
        VkBuffer UniformBuffer {VK_NULL_HANDLE};
        void* UniformBufferData {nullptr};
        std::unordered_map<TextureType, TextureBufferData> Textures {};
    };
}// namespace RenderCore
