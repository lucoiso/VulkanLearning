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
        glm::vec2 UV{};
        glm::vec3 Position{};
        glm::vec3 Normal{};
        glm::vec4 Color{};
        glm::vec4 Joint{};
        glm::vec4 Weight{};
        glm::vec4 Tangent {};
    };

    export struct RENDERCOREMODULE_API Meshlet
    {
        glm::uint IndexCount{ 0U };
        glm::uint VertexCount{ 0U };
        glm::uint IndexOffset{ 0U };
        glm::uint VertexOffset{ 0U };
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
