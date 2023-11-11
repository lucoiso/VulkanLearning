// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <volk.h>

export module RenderCore.Types.MeshBufferData;

export import <cstdint>;
export import <string>;
export import <unordered_map>;

export import RenderCore.Types.TextureBufferData;

namespace RenderCore
{
    export struct MeshBufferData
    {
        std::uint32_t const ID {0U};
        VkBuffer UniformBuffer {VK_NULL_HANDLE};
        void* UniformBufferData {nullptr};
        std::unordered_map<TextureType, TextureBufferData> Textures {};
    };
}// namespace RenderCore
