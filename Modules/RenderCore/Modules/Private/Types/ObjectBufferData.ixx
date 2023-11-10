// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <volk.h>

export module RenderCore.Types.ObjectBufferData;

export import <cstdint>;
export import <string>;
export import <vector>;

export import RenderCore.Types.TextureBufferData;

namespace RenderCore
{
    export struct ObjectBufferData
    {
        std::uint32_t const ObjectID {0U};
        VkBuffer UniformBuffer {VK_NULL_HANDLE};
        void* UniformBufferData {nullptr};
        std::vector<TextureBufferData> Textures {};
    };
}// namespace RenderCore
