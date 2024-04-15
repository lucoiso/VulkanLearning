// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <Volk/volk.h>
#include <glm/ext.hpp>

export module RenderCore.Types.Vertex;

namespace RenderCore
{
        export struct Vertex
        {
                glm::vec3 Position {};
                glm::vec3 Normal {};
                glm::vec2 TextureCoordinate {};
                glm::vec4 Color {};
                glm::vec4 Joint {};
                glm::vec4 Weight {};
                glm::vec4 Tangent {};
        };

        export namespace VertexAttributes
        {
                constexpr VkVertexInputAttributeDescription Position {
                                .format = VK_FORMAT_R32G32B32_SFLOAT,
                                .offset = static_cast<std::uint32_t>(offsetof(Vertex, Position))
                };

                constexpr VkVertexInputAttributeDescription Normal {
                                .format = VK_FORMAT_R32G32B32_SFLOAT,
                                .offset = static_cast<std::uint32_t>(offsetof(Vertex, Normal))
                };

                constexpr VkVertexInputAttributeDescription TextureCoordinate {
                                .format = VK_FORMAT_R32G32_SFLOAT,
                                .offset = static_cast<std::uint32_t>(offsetof(Vertex, TextureCoordinate))
                };

                constexpr VkVertexInputAttributeDescription Color {
                                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                                .offset = static_cast<std::uint32_t>(offsetof(Vertex, Color))
                };

                constexpr VkVertexInputAttributeDescription Joint {
                                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                                .offset = static_cast<std::uint32_t>(offsetof(Vertex, Joint))
                };

                constexpr VkVertexInputAttributeDescription Weight {
                                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                                .offset = static_cast<std::uint32_t>(offsetof(Vertex, Weight))
                };

                constexpr VkVertexInputAttributeDescription Tangent {
                                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                                .offset = static_cast<std::uint32_t>(offsetof(Vertex, Tangent))
                };
        } // namespace VertexAttributes
}         // namespace RenderCore
