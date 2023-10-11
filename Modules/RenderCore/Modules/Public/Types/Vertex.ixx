// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <RenderCoreModule.h>

export module RenderCore.Types.Vertex;

import <array>;

namespace RenderCore
{
    export struct RENDERCOREMODULE_API Vertex
    {
        std::array<float, 3U> Position {};
        std::array<float, 3U> Normal {};
        std::array<float, 3U> Color {};
        std::array<float, 2U> TextureCoordinate {};
    };
}// namespace RenderCore
