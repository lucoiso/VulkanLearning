// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <tiny_gltf.h>

export module RenderCore.Factories.Mesh;

import RenderCore.Types.Mesh;
import RenderCore.Types.Texture;

namespace RenderCore
{
    export struct MeshConstructionInputParameters
    {
        std::uint32_t                                                      ID { 0U };
        strzilla::string_view const &                                           Path {};
        tinygltf::Model const &                                            Model {};
        tinygltf::Node const &                                             Node {};
        tinygltf::Mesh const &                                             Mesh {};
        tinygltf::Primitive const &                                        Primitive {};
        std::unordered_map<std::uint32_t, std::shared_ptr<Texture>> const &TextureMap {};
    };

    export [[nodiscard]] std::shared_ptr<Mesh> ConstructMesh(MeshConstructionInputParameters const &);
}; // namespace RenderCore
