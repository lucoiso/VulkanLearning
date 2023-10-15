// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <volk.h>

export module RenderCore.Types.ObjectData;

import <cstdint>;

namespace RenderCore
{
    export struct ObjectData
    {
        std::uint32_t const ObjectID {0U};
        VkBuffer UniformBuffer {VK_NULL_HANDLE};
        void* UniformBufferData {nullptr};
    };
}// namespace RenderCore
