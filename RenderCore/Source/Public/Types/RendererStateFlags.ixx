// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <cstdint>

export module RenderCore.Types.RendererStateFlags;

namespace RenderCore
{
    export enum class RendererStateFlags : std::uint8_t
    {
        NONE                             = 0,
        INITIALIZED                      = 1 << 0,
        PENDING_DEVICE_PROPERTIES_UPDATE = 1 << 1,
        PENDING_RESOURCES_DESTRUCTION    = 1 << 2,
        PENDING_RESOURCES_CREATION       = 1 << 3,
        PENDING_PIPELINE_REFRESH         = 1 << 4,
        PENDING_COMMANDS_ALLOCATION      = 1 << 5,
    };

    export enum class RendererObjectsManagementStateFlags : std::uint8_t
    {
        NONE           = 0,
        PENDING_LOAD   = 1 << 0,
        PENDING_UNLOAD = 1 << 1,
        PENDING_CLEAR  = 1 << 2,
    };
} // namespace RenderCore
