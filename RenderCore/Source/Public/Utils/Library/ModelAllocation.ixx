// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <tiny_gltf.h>
#include <vma/vk_mem_alloc.h>

export module RenderCore.Runtime.Buffer.ModelAllocation;

import RenderCore.Types.Vertex;
import RenderCore.Types.TextureType;
import RenderCore.Types.Object;
import RenderCore.Types.Allocation;

export namespace RenderCore
{
    void TryResizeVertexContainer(std::vector<Vertex> &, std::uint32_t);

    void InsertIndiceInContainer(std::vector<std::uint32_t> &, tinygltf::Accessor const &, auto const *);

    float const *GetPrimitiveData(ObjectData &, std::string_view const &, tinygltf::Model const &, tinygltf::Primitive const &, std::uint32_t *);

    void AllocateModelTexture(ObjectData &, tinygltf::Model const &, VmaAllocator const &, std::int32_t, VkFormat, TextureType);

    void AllocateVertexAttributes(ObjectData &, tinygltf::Model const &, tinygltf::Primitive const &);

    std::uint32_t AllocatePrimitiveIndices(ObjectData &, tinygltf::Model const &, tinygltf::Primitive const &);

    void SetPrimitiveTransform(Object &, tinygltf::Node const &);

    void AllocatePrimitiveMaterials(ObjectData &, tinygltf::Model const &, tinygltf::Primitive const &, VmaAllocator const &, VkFormat);
} // namespace RenderCore
