// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Runtime.Model;

import RenderCore.Types.Vertex;
import RenderCore.Types.Transform;

namespace RenderCore
{
    void         InsertIndicesInContainer(std::vector<std::uint32_t> &, tinygltf::Accessor const &, auto const *);
    float const *GetPrimitiveData(strzilla::string_view const &, tinygltf::Model const &, tinygltf::Primitive const &, std::uint32_t *);
    export std::vector<Vertex> SetVertexAttributes(tinygltf::Model const &, tinygltf::Primitive const &);
    export std::vector<std::uint32_t> AllocatePrimitiveIndices(tinygltf::Model const &, tinygltf::Primitive const &);
    export Transform GetPrimitiveTransform(tinygltf::Node const &);
} // namespace RenderCore
