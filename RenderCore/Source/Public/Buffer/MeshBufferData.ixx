// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <cstdint>
#include <unordered_map>
#include <Volk/volk.h>

export module RenderCore.Types.MeshBufferData;

import RenderCore.Types.TextureType;

namespace RenderCore
{
    export struct MeshBufferData
    {
        std::uint32_t ID{0U};
        VkBuffer UniformBuffer{VK_NULL_HANDLE};
        void* UniformBufferData{nullptr};
        std::unordered_map<TextureType, VkImageView> Textures{};
    };
}// namespace RenderCore
