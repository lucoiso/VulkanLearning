// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

#pragma once

#include <cstdint>

namespace RenderCore
{
    enum class VulkanQueueType : std::uint8_t
    {
        Graphics,
        Compute,
        Transfer,
        Present
    };
}
