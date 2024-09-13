// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Runtime.Model;

export import RenderCore.Types.Mesh;

namespace RenderCore
{
    void         InsertIndiceInContainer(std::vector<std::uint32_t> &, tinygltf::Accessor const &, auto const *);
    float const *GetPrimitiveData(strzilla::string_view const &, tinygltf::Model const &, tinygltf::Primitive const &, std::uint32_t *);
    export void  SetVertexAttributes(std::shared_ptr<Mesh> const &, tinygltf::Model const &, tinygltf::Primitive const &);
    export void  AllocatePrimitiveIndices(std::shared_ptr<Mesh> const &, tinygltf::Model const &, tinygltf::Primitive const &);
    export void  SetPrimitiveTransform(std::shared_ptr<Mesh> const &, tinygltf::Node const &);
} // namespace RenderCore
