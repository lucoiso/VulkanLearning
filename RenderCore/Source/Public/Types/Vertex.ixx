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
        alignas(8)  glm::vec2 UV{};
        alignas(8)  glm::float32 _Padding1{};
        alignas(16) glm::vec3 Position{};
        alignas(16) glm::vec3 Normal{};
        alignas(16) glm::vec4 Color{};
        alignas(16) glm::vec4 Joint{};
        alignas(16) glm::vec4 Weight{};
        alignas(16) glm::vec4 Tangent{};
    };

    export struct RENDERCOREMODULE_API Meshlet
    {
        alignas(4) glm::uint VertexOffset{ 0U };
        alignas(4) glm::uint TriangleOffset{ 0U };
        alignas(4) glm::uint VertexCount{ 0U };
        alignas(4) glm::uint TriangleCount{ 0U };
    };

    export namespace VertexAttributes
    {
        constexpr VkVertexInputAttributeDescription Position {
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = static_cast<glm::uint>(offsetof(Vertex, Position))
        };

        constexpr VkVertexInputAttributeDescription Normal {
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = static_cast<glm::uint>(offsetof(Vertex, Normal))
        };

        constexpr VkVertexInputAttributeDescription UV {
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = static_cast<glm::uint>(offsetof(Vertex, UV))
        };

        constexpr VkVertexInputAttributeDescription Color {
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offset = static_cast<glm::uint>(offsetof(Vertex, Color))
        };

        constexpr VkVertexInputAttributeDescription Joint {
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offset = static_cast<glm::uint>(offsetof(Vertex, Joint))
        };

        constexpr VkVertexInputAttributeDescription Weight {
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offset = static_cast<glm::uint>(offsetof(Vertex, Weight))
        };

        constexpr VkVertexInputAttributeDescription Tangent {
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offset = static_cast<glm::uint>(offsetof(Vertex, Tangent))
        };
    } // namespace VertexAttributes
} // namespace RenderCore
