// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <tiny_gltf.h>
#include <vma/vk_mem_alloc.h>

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
}; // namespace RenderCore
