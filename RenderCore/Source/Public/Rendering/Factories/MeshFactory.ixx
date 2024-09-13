// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Factories.Mesh;

export import RenderCore.Types.Mesh;

namespace RenderCore
{
    export struct RENDERCOREMODULE_API MeshConstructionInputParameters
    {
        std::uint32_t                                                      ID { 0U };
        strzilla::string_view const &                                      Path {};
        tinygltf::Model const &                                            Model {};
        tinygltf::Node const &                                             Node {};
        tinygltf::Mesh const &                                             Mesh {};
        tinygltf::Primitive const &                                        Primitive {};
        std::unordered_map<std::uint32_t, std::shared_ptr<Texture>> const &TextureMap {};
    };

    export RENDERCOREMODULE_API [[nodiscard]] std::shared_ptr<Mesh> ConstructMesh(MeshConstructionInputParameters const &);
}