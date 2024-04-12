// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <tiny_gltf.h>
#include <unordered_map>
#include <memory>
#include <vma/vk_mem_alloc.h>

export module RenderCore.Runtime.Model;

import RenderCore.Types.Vertex;
import RenderCore.Types.Material;
import RenderCore.Types.Mesh;
import RenderCore.Types.Allocation;

export namespace RenderCore
{
    void InsertIndiceInContainer(std::vector<std::uint32_t> &, tinygltf::Accessor const &, auto const *);

    float const *GetPrimitiveData(std::string_view const &, tinygltf::Model const &, tinygltf::Primitive const &, std::uint32_t *);

    void SetVertexAttributes(std::shared_ptr<Mesh> const &, tinygltf::Model const &, tinygltf::Primitive const &);

    void AllocatePrimitiveIndices(std::shared_ptr<Mesh> const &, tinygltf::Model const &, tinygltf::Primitive const &);

    void SetPrimitiveTransform(std::shared_ptr<Mesh> const &, tinygltf::Node const &);
} // namespace RenderCore
