// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include "RenderCoreModule.h"

export module RenderCore.Types.Vertex;

import <array>;

export namespace RenderCore
{
    struct RENDERCOREMODULE_API Vertex
    {
        std::array<float, 3U> Position {};
        std::array<float, 3U> Color {};
        std::array<float, 2U> TextureCoordinate {};
    };
}// namespace RenderCore
