// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <boost/log/trivial.hpp>
#include <filesystem>
#include <glm/ext.hpp>
#include <tiny_gltf.h>
#include <vma/vk_mem_alloc.h>
#include "Utils/Library/Macros.h"

module RenderCore.Runtime.Buffer.ModelAllocation;

import RuntimeInfo.Manager;
import RenderCore.Runtime.Buffer.Operations;
import RenderCore.Types.Transform;

using namespace RenderCore;

void RenderCore::TryResizeVertexContainer(std::vector<Vertex> &Vertices, std::uint32_t const NewSize)
{
    PUSH_CALLSTACK();

    if (std::size(Vertices) < NewSize && NewSize > 0U)
    {
        Vertices.resize(NewSize);
    }
}

void RenderCore::InsertIndiceInContainer(std::vector<std::uint32_t> &Indices, tinygltf::Accessor const &IndexAccessor, auto const *Data)
{
    PUSH_CALLSTACK();

    for (std::uint32_t Iterator = 0U; Iterator < IndexAccessor.count; ++Iterator)
    {
        Indices.push_back(static_cast<std::uint32_t>(Data[Iterator]));
    }
}

float const *RenderCore::GetPrimitiveData(ObjectData                &ObjectCreationData,
                                          std::string_view const    &ID,
                                          tinygltf::Model const     &Model,
                                          tinygltf::Primitive const &Primitive,
                                          std::uint32_t             *NumComponents = nullptr)
{
    PUSH_CALLSTACK();

    if (Primitive.attributes.contains(std::data(ID)))
    {
        tinygltf::Accessor const   &Accessor   = Model.accessors.at(Primitive.attributes.at(std::data(ID)));
        tinygltf::BufferView const &BufferView = Model.bufferViews.at(Accessor.bufferView);
        tinygltf::Buffer const     &Buffer     = Model.buffers.at(BufferView.buffer);

        if (NumComponents)
        {
            switch (Accessor.type)
            {
                case TINYGLTF_TYPE_SCALAR:
                    *NumComponents = 1U;
                    break;
                case TINYGLTF_TYPE_VEC2:
                    *NumComponents = 2U;
                    break;
                case TINYGLTF_TYPE_VEC3:
                    *NumComponents = 3U;
                    break;
                case TINYGLTF_TYPE_VEC4:
                    *NumComponents = 4U;
                    break;
                default:
                    break;
            }
        }

        TryResizeVertexContainer(ObjectCreationData.Vertices, Accessor.count);
        return reinterpret_cast<float const *>(std::data(Buffer.data) + BufferView.byteOffset + Accessor.byteOffset);
    }

    return nullptr;
}

void RenderCore::AllocateModelTexture(ObjectData            &ObjectCreationData,
                                      tinygltf::Model const &Model,
                                      VmaAllocator const    &Allocator,
                                      std::int32_t const     TextureIndex,
                                      VkFormat const         ImageFormat,
                                      TextureType const      TextureType)
{
    PUSH_CALLSTACK();

    if (TextureIndex >= 0)
    {
        if (tinygltf::Texture const &Texture = Model.textures.at(TextureIndex); Texture.source >= 0)
        {
            tinygltf::Image const &Image = Model.images.at(Texture.source);

            ObjectCreationData.ImageCreationDatas.push_back(
                AllocateTexture(Allocator, std::data(Image.image), Image.width, Image.height, ImageFormat, std::size(Image.image)));
            ObjectCreationData.ImageCreationDatas.back().Type = TextureType;
        }
    }
}

void RenderCore::AllocateVertexAttributes(ObjectData &Allocation, tinygltf::Model const &Model, tinygltf::Primitive const &Primitive)
{
    PUSH_CALLSTACK();

    const float  *PositionData = GetPrimitiveData(Allocation, "POSITION", Model, Primitive);
    const float  *NormalData   = GetPrimitiveData(Allocation, "NORMAL", Model, Primitive);
    const float  *TexCoordData = GetPrimitiveData(Allocation, "TEXCOORD_0", Model, Primitive);
    std::uint32_t NumColorComponents {};
    const float  *ColorData   = GetPrimitiveData(Allocation, "COLOR_0", Model, Primitive, &NumColorComponents);
    const float  *JointData   = GetPrimitiveData(Allocation, "JOINTS_0", Model, Primitive);
    const float  *WeightData  = GetPrimitiveData(Allocation, "WEIGHTS_0", Model, Primitive);
    const float  *TangentData = GetPrimitiveData(Allocation, "TANGENT", Model, Primitive);

    bool const HasSkin = JointData && WeightData;

    for (std::uint32_t Iterator = 0U; Iterator < static_cast<std::uint32_t>(std::size(Allocation.Vertices)); ++Iterator)
    {
        if (PositionData)
        {
            Allocation.Vertices.at(Iterator).Position = glm::make_vec3(&PositionData[Iterator * 3]);
        }

        if (NormalData)
        {
            Allocation.Vertices.at(Iterator).Normal = glm::make_vec3(&NormalData[Iterator * 3]);
        }

        if (TexCoordData)
        {
            Allocation.Vertices.at(Iterator).TextureCoordinate = glm::make_vec2(&TexCoordData[Iterator * 2]);
        }

        if (ColorData)
        {
            switch (NumColorComponents)
            {
                case 3U:
                    Allocation.Vertices.at(Iterator).Color = glm::vec4(glm::make_vec3(&ColorData[Iterator * 3]), 1.F);
                    break;
                case 4U:
                    Allocation.Vertices.at(Iterator).Color = glm::make_vec4(&ColorData[Iterator * 4]);
                    break;
                default:
                    break;
            }
        }
        else
        {
            Allocation.Vertices.at(Iterator).Color = glm::vec4(1.F);
        }

        if (HasSkin)
        {
            Allocation.Vertices.at(Iterator).Joint  = glm::make_vec4(&JointData[Iterator * 4]);
            Allocation.Vertices.at(Iterator).Weight = glm::make_vec4(&WeightData[Iterator * 4]);
        }

        if (TangentData)
        {
            Allocation.Vertices.at(Iterator).Tangent = glm::make_vec4(&TangentData[Iterator * 4]);
        }
    }
}

std::uint32_t RenderCore::AllocatePrimitiveIndices(ObjectData &ObjectCreationData, tinygltf::Model const &Model, tinygltf::Primitive const &Primitive)
{
    PUSH_CALLSTACK();

    if (Primitive.indices >= 0)
    {
        tinygltf::Accessor const   &IndexAccessor   = Model.accessors.at(Primitive.indices);
        tinygltf::BufferView const &IndexBufferView = Model.bufferViews.at(IndexAccessor.bufferView);
        tinygltf::Buffer const     &IndexBuffer     = Model.buffers.at(IndexBufferView.buffer);
        unsigned char const *const  IndicesData     = std::data(IndexBuffer.data) + IndexBufferView.byteOffset + IndexAccessor.byteOffset;

        ObjectCreationData.Indices.reserve(IndexAccessor.count);

        switch (IndexAccessor.componentType)
        {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
            {
                InsertIndiceInContainer(ObjectCreationData.Indices, IndexAccessor, reinterpret_cast<const uint32_t *>(IndicesData));
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
            {
                InsertIndiceInContainer(ObjectCreationData.Indices, IndexAccessor, reinterpret_cast<const uint16_t *>(IndicesData));
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
            {
                InsertIndiceInContainer(ObjectCreationData.Indices, IndexAccessor, IndicesData);
                break;
            }
            default:
                break;
        }

        return IndexAccessor.count / 3U;
    }

    return 0U;
}

void RenderCore::SetPrimitiveTransform(Object &Object, tinygltf::Node const &Node)
{
    PUSH_CALLSTACK();

    if (!std::empty(Node.translation))
    {
        Object.SetPosition(Vector(glm::make_vec3(std::data(Node.translation))));
    }

    if (!std::empty(Node.scale))
    {
        Object.SetScale(Vector(glm::make_vec3(std::data(Node.scale))));
    }

    if (!std::empty(Node.rotation))
    {
        Object.SetRotation(Rotator(glm::make_quat(std::data(Node.rotation))));
    }
}

void RenderCore::AllocatePrimitiveMaterials(ObjectData                &ObjectCreationData,
                                            tinygltf::Model const     &Model,
                                            tinygltf::Primitive const &Primitive,
                                            VmaAllocator const        &Allocator,
                                            VkFormat const             SwapChainImageFormat)
{
    PUSH_CALLSTACK();

    if (Primitive.material >= 0)
    {
        tinygltf::Material const &Material = Model.materials.at(Primitive.material);

        if (Material.pbrMetallicRoughness.baseColorTexture.index >= 0)
        {
            AllocateModelTexture(ObjectCreationData,
                                 Model,
                                 Allocator,
                                 Material.pbrMetallicRoughness.baseColorTexture.index,
                                 SwapChainImageFormat,
                                 TextureType::BaseColor);
        }

        if (Material.normalTexture.index >= 0)
        {
            AllocateModelTexture(ObjectCreationData, Model, Allocator, Material.normalTexture.index, SwapChainImageFormat, TextureType::Normal);
        }

        if (Material.occlusionTexture.index >= 0)
        {
            AllocateModelTexture(ObjectCreationData, Model, Allocator, Material.occlusionTexture.index, SwapChainImageFormat, TextureType::Occlusion);
        }

        if (Material.emissiveTexture.index >= 0)
        {
            AllocateModelTexture(ObjectCreationData, Model, Allocator, Material.emissiveTexture.index, SwapChainImageFormat, TextureType::Emissive);
        }

        if (Material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
        {
            AllocateModelTexture(ObjectCreationData,
                                 Model,
                                 Allocator,
                                 Material.pbrMetallicRoughness.metallicRoughnessTexture.index,
                                 SwapChainImageFormat,
                                 TextureType::MetallicRoughness);
        }
    }
}
