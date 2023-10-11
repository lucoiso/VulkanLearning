// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <RenderCoreModule.h>

export module RenderCore.Types.UniformBufferObject;

import <array>;

constexpr std::uint32_t MATRIX_SIZE = 4U;

namespace RenderCore
{
    export struct RENDERCOREMODULE_API UniformBufferObject
    {
        std::array<std::array<float, MATRIX_SIZE>, MATRIX_SIZE> ModelViewProjection {};

        UniformBufferObject& operator=(std::array<std::array<float, MATRIX_SIZE>, MATRIX_SIZE> const& value)
        {
            ModelViewProjection = value;

            return *this;
        }

        UniformBufferObject& operator=(float value)
        {
            for (std::array<float, MATRIX_SIZE>& Column: ModelViewProjection)
            {
                for (float& Line: Column)
                {
                    Line = value;
                }
            }

            return *this;
        }
    };
}// namespace RenderCore
