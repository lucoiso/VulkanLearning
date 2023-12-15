// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

export module RenderCore.Utils.EngineStateFlags;

import <cstdint>;
import RenderCore.Utils.EnumHelpers;

namespace RenderCore
{
    export enum class RendererStateFlags : std::uint8_t {
        NONE                             = 0,
        INITIALIZED                      = 1 << 0,
        PENDING_DEVICE_PROPERTIES_UPDATE = 1 << 1,
        PENDING_RESOURCES_DESTRUCTION    = 1 << 2,
        PENDING_RESOURCES_CREATION       = 1 << 3,
        PENDING_PIPELINE_REFRESH         = 1 << 4,
    };
}// namespace RenderCore