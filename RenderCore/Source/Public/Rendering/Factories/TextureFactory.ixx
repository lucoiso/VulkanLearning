// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Factories.Texture;

import RenderCore.Types.Texture;

namespace RenderCore
{
    export struct TextureConstructionInputParameters
    {
        std::uint32_t          ID { 0U };
        tinygltf::Image const &Image {};

        VkCommandBuffer AllocationCmdBuffer { VK_NULL_HANDLE };
    };

    export struct TextureConstructionOutputParameters
    {
        VkBuffer      StagingBuffer {};
        VmaAllocation StagingAllocation {};
    };

    export [[nodiscard]] std::shared_ptr<Texture> ConstructTexture(TextureConstructionInputParameters const &, TextureConstructionOutputParameters &);

    export [[nodiscard]] std::shared_ptr<Texture> ConstructTextureFromFile(strzilla::string_view const &, VkCommandBuffer&, TextureConstructionOutputParameters &);
}; // namespace RenderCore
