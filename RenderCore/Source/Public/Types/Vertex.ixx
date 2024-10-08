// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Types.Vertex;

import RenderCore.Utils.Constants;

namespace RenderCore
{
    export struct RENDERCOREMODULE_API alignas(16) Vertex
    {
        alignas(8)  glm::vec2 TextureCoordinate{};
        alignas(16) glm::vec3 Position {};
        alignas(16) glm::vec3 Normal {};
        alignas(16) glm::vec4 Color {};
        alignas(16) glm::vec4 Joint {};
        alignas(16) glm::vec4 Weight {};
        alignas(16) glm::vec4 Tangent {};
    };

    export struct RENDERCOREMODULE_API alignas(16) Meshlet
    {
        alignas(4)  std::uint32_t VertexCount{ 0U };
        alignas(4)  std::uint32_t IndexCount{ 0U };
        alignas(4)  std::array<std::uint32_t, g_MaxMeshletPrimitives * 3U> Indices {};
        alignas(16) std::array<Vertex, g_MaxMeshletVertices> Vertices {};
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
} // namespace RenderCore
