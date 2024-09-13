// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Factories.Texture;

import RenderCore.Types.Texture;

namespace RenderCore
{
    export struct RENDERCOREMODULE_API TextureConstructionInputParameters
    {
        std::uint32_t          ID { 0U };
        tinygltf::Image const &Image {};

        VkCommandBuffer AllocationCmdBuffer { VK_NULL_HANDLE };
    };

    export struct RENDERCOREMODULE_API TextureConstructionOutputParameters
    {
        VkBuffer      StagingBuffer {};
        VmaAllocation StagingAllocation {};
    };

    export RENDERCOREMODULE_API [[nodiscard]] std::shared_ptr<Texture> ConstructTexture(TextureConstructionInputParameters const &, TextureConstructionOutputParameters &);

    export RENDERCOREMODULE_API [[nodiscard]] std::shared_ptr<Texture> ConstructTextureFromFile(strzilla::string_view const &, VkCommandBuffer&, TextureConstructionOutputParameters &);
}