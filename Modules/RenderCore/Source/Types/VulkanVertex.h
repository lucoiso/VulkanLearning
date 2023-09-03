// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include <vulkan/vulkan.h>
#include <array>

namespace RenderCore
{
    struct Vertex
    {
        std::uint16_t Index = 0u;
        std::array<float, 3> Position{ 0.f, 0.f, 0.f };
        std::array<float, 4> Color{ 0.f, 0.f, 0.f, 1.f };

        static inline std::array<VkVertexInputBindingDescription, 1> GetBindingDescriptors()
        {
            return {
                VkVertexInputBindingDescription{
                    .binding = 0u,
                    .stride = sizeof(Vertex),
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
                }
            };
        }

        static inline std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions() {
            return {
                VkVertexInputAttributeDescription{
                    .location = 0u,
                    .binding = 0u,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = offsetof(Vertex, Position)
                },
                VkVertexInputAttributeDescription{
                    .location = 1u,
                    .binding = 0u,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = offsetof(Vertex, Color)
                }
            };
        }
    };
}