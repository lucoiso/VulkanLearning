// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <tiny_gltf.h>
#include <unordered_map>
#include <vma/vk_mem_alloc.h>

export module RenderCore.Runtime.Model;

import RenderCore.Types.Vertex;
import RenderCore.Types.Material;
import RenderCore.Types.Object;
import RenderCore.Types.Allocation;

export namespace RenderCore
{
    void InsertIndiceInContainer(std::vector<std::uint32_t> &, tinygltf::Accessor const &, auto const *);

    float const *GetPrimitiveData(std::string_view const &, tinygltf::Model const &, tinygltf::Primitive const &, std::uint32_t *);

    void SetVertexAttributes(Object &, tinygltf::Model const &, tinygltf::Primitive const &);

    void AllocatePrimitiveIndices(Object &, tinygltf::Model const &, tinygltf::Primitive const &);

    void SetPrimitiveTransform(Object &, tinygltf::Node const &);

    std::unordered_map<VkBuffer, VmaAllocation> AllocateObjectMaterials(VkCommandBuffer &,
                                                                        Object &,
                                                                        tinygltf::Primitive const &,
                                                                        tinygltf::Model const &);
} // namespace RenderCore
