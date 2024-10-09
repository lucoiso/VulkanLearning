// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Types.Vertex;

import RenderCore.Utils.Constants;

namespace RenderCore
{
    export struct RENDERCOREMODULE_API Vertex
    {
        glm::vec2 TextureCoordinate{};
        glm::vec3 Position {};
        glm::vec3 Normal {};
        glm::vec4 Color {};
        glm::vec4 Joint {};
        glm::vec4 Weight {};
        glm::vec4 Tangent {};
    };

    export struct RENDERCOREMODULE_API Meshlet
    {
        std::uint32_t VertexCount{ 0U };
        std::uint32_t IndexCount{ 0U };
        std::array<Vertex, g_MaxMeshletVertices> Vertices {};
        std::array<std::uint32_t, g_MaxMeshletIndices> Indices{};
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
