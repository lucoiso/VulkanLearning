// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <filesystem>
#include <tiny_gltf.h>
#include <glm/ext.hpp>
#include <vma/vk_mem_alloc.h>

module RenderCore.Runtime.Model;

import RenderCore.Runtime.Memory;
import RenderCore.Runtime.SwapChain;
import RenderCore.Runtime.Scene;
import RenderCore.Types.Transform;

using namespace RenderCore;

void RenderCore::InsertIndiceInContainer(std::vector<std::uint32_t> &Indices, tinygltf::Accessor const &IndexAccessor, auto const *Data)
{
    for (std::uint32_t Iterator = 0U; Iterator < IndexAccessor.count; ++Iterator)
    {
        Indices.push_back(static_cast<std::uint32_t>(Data[Iterator]));
    }
}

float const *RenderCore::GetPrimitiveData(std::string_view const &   ID,
                                          tinygltf::Model const &    Model,
                                          tinygltf::Primitive const &Primitive,
                                          std::uint32_t *            NumComponents = nullptr)
{
    if (Primitive.attributes.contains(std::data(ID)))
    {
        tinygltf::Accessor const &  Accessor   = Model.accessors.at(Primitive.attributes.at(std::data(ID)));
        tinygltf::BufferView const &BufferView = Model.bufferViews.at(Accessor.bufferView);
        tinygltf::Buffer const &    Buffer     = Model.buffers.at(BufferView.buffer);

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

        return reinterpret_cast<float const *>(std::data(Buffer.data) + BufferView.byteOffset + Accessor.byteOffset);
    }

    return nullptr;
}

void RenderCore::SetVertexAttributes(Object &Object, tinygltf::Model const &Model, tinygltf::Primitive const &Primitive)
{
    std::vector<Vertex> Vertices;
    {
        tinygltf::Accessor const &PositionAccessor = Model.accessors.at(Primitive.attributes.at("POSITION"));
        Vertices.resize(PositionAccessor.count);
    }

    const float * PositionData = GetPrimitiveData("POSITION", Model, Primitive);
    const float * NormalData   = GetPrimitiveData("NORMAL", Model, Primitive);
    const float * TexCoordData = GetPrimitiveData("TEXCOORD_0", Model, Primitive);
    std::uint32_t NumColorComponents {};
    const float * ColorData   = GetPrimitiveData("COLOR_0", Model, Primitive, &NumColorComponents);
    const float * JointData   = GetPrimitiveData("JOINTS_0", Model, Primitive);
    const float * WeightData  = GetPrimitiveData("WEIGHTS_0", Model, Primitive);
    const float * TangentData = GetPrimitiveData("TANGENT", Model, Primitive);

    bool const HasSkin = JointData && WeightData;

    for (std::uint32_t Iterator = 0U; Iterator < static_cast<std::uint32_t>(std::size(Vertices)); ++Iterator)
    {
        if (PositionData)
        {
            Vertices.at(Iterator).Position = glm::make_vec3(&PositionData[Iterator * 3]);
        }

        if (NormalData)
        {
            Vertices.at(Iterator).Normal = glm::make_vec3(&NormalData[Iterator * 3]);
        }

        if (TexCoordData)
        {
            Vertices.at(Iterator).TextureCoordinate = glm::make_vec2(&TexCoordData[Iterator * 2]);
        }

        if (ColorData)
        {
            switch (NumColorComponents)
            {
                case 3U:
                    Vertices.at(Iterator).Color = glm::vec4(glm::make_vec3(&ColorData[Iterator * 3]), 1.F);
                    break;
                case 4U:
                    Vertices.at(Iterator).Color = glm::make_vec4(&ColorData[Iterator * 4]);
                    break;
                default:
                    break;
            }
        }
        else
        {
            Vertices.at(Iterator).Color = glm::vec4(1.F);
        }

        if (HasSkin)
        {
            Vertices.at(Iterator).Joint  = glm::make_vec4(&JointData[Iterator * 4]);
            Vertices.at(Iterator).Weight = glm::make_vec4(&WeightData[Iterator * 4]);
        }

        if (TangentData)
        {
            Vertices.at(Iterator).Tangent = glm::make_vec4(&TangentData[Iterator * 4]);
        }
    }

    Object.SetVertexBuffer(Vertices);
}

void RenderCore::AllocatePrimitiveIndices(Object &Object, tinygltf::Model const &Model, tinygltf::Primitive const &Primitive)
{
    std::vector<std::uint32_t> Indices;

    if (Primitive.indices >= 0)
    {
        tinygltf::Accessor const &  IndexAccessor   = Model.accessors.at(Primitive.indices);
        tinygltf::BufferView const &IndexBufferView = Model.bufferViews.at(IndexAccessor.bufferView);
        tinygltf::Buffer const &    IndexBuffer     = Model.buffers.at(IndexBufferView.buffer);
        unsigned char const *const  IndicesData     = std::data(IndexBuffer.data) + IndexBufferView.byteOffset + IndexAccessor.byteOffset;

        Indices.reserve(IndexAccessor.count);

        switch (IndexAccessor.componentType)
        {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
            {
                InsertIndiceInContainer(Indices, IndexAccessor, reinterpret_cast<const uint32_t *>(IndicesData));
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
            {
                InsertIndiceInContainer(Indices, IndexAccessor, reinterpret_cast<const uint16_t *>(IndicesData));
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
            {
                InsertIndiceInContainer(Indices, IndexAccessor, IndicesData);
                break;
            }
            default:
                break;
        }
    }

    Object.SetIndexBuffer(Indices);
}

void RenderCore::SetPrimitiveTransform(Object &Object, tinygltf::Node const &Node)
{
    if (!std::empty(Node.translation))
    {
        Object.SetPosition(glm::make_vec3(std::data(Node.translation)));
    }

    if (!std::empty(Node.scale))
    {
        Object.SetScale(glm::make_vec3(std::data(Node.scale)));
    }

    if (!std::empty(Node.rotation))
    {
        Object.SetRotation(degrees(eulerAngles(glm::make_quat(std::data(Node.rotation)))));
    }

    if (!Node.matrix.empty())
    {
        Object.SetMatrix(glm::make_mat4(std::data(Node.matrix)));
    }
}

std::unordered_map<VkBuffer, VmaAllocation> RenderCore::AllocateObjectBuffers(VkCommandBuffer const &CommandBuffer, Object &Object)
{
    std::unordered_map Output {
            CreateVertexBuffers(CommandBuffer, Object.GetMutableAllocationData(), Object.GetVertices()),
            CreateIndexBuffers(CommandBuffer, Object.GetMutableAllocationData(), Object.GetIndices())
    };

    CreateModelUniformBuffers(Object);

    return Output;
}

std::unordered_map<VkBuffer, VmaAllocation> RenderCore::AllocateObjectMaterials(VkCommandBuffer &          CommandBuffer,
                                                                                Object &                   Object,
                                                                                tinygltf::Primitive const &Primitive,
                                                                                tinygltf::Model const &    Model)
{
    std::unordered_map<VkBuffer, VmaAllocation> Output;

    if (Primitive.material >= 0)
    {
        tinygltf::Material const &Material = Model.materials.at(Primitive.material);

        Object.SetMaterialData({
                                       .BaseColorFactor = glm::make_vec4(std::data(Material.pbrMetallicRoughness.baseColorFactor)),
                                       .EmissiveFactor = glm::make_vec3(std::data(Material.emissiveFactor)),
                                       .MetallicFactor = static_cast<float>(Material.pbrMetallicRoughness.metallicFactor),
                                       .RoughnessFactor = static_cast<float>(Material.pbrMetallicRoughness.roughnessFactor),
                                       .AlphaCutoff = static_cast<float>(Material.alphaCutoff),
                                       .NormalScale = static_cast<float>(Material.normalTexture.scale),
                                       .OcclusionStrength = static_cast<float>(Material.occlusionTexture.strength),
                                       .AlphaMode = Material.alphaMode == "OPAQUE"
                                                        ? AlphaMode::ALPHA_OPAQUE
                                                        : Material.alphaMode == "MASK"
                                                              ? AlphaMode::ALPHA_MASK
                                                              : AlphaMode::ALPHA_BLEND,
                                       .DoubleSided = Material.doubleSided
                               });

        if (Material.pbrMetallicRoughness.baseColorTexture.index >= 0)
        {
            Output.emplace(AllocateTexture(CommandBuffer,
                                           std::data(Model.images.at(Model.textures.at(Material.pbrMetallicRoughness.baseColorTexture.index).source).
                                                           image),
                                           Model.images.at(Model.textures.at(Material.pbrMetallicRoughness.baseColorTexture.index).source).width,
                                           Model.images.at(Model.textures.at(Material.pbrMetallicRoughness.baseColorTexture.index).source).height,
                                           GetSwapChainImageFormat(),
                                           std::size(Model.images.at(Model.textures.at(Material.pbrMetallicRoughness.baseColorTexture.index).source).
                                                           image),
                                           Object.GetMutableAllocationData().TextureImageAllocations.emplace_back()));

            Object.GetMutableAllocationData().TextureDescriptors.emplace(TextureType::BaseColor,
                                                                         VkDescriptorImageInfo {
                                                                                 .sampler = GetSampler(),
                                                                                 .imageView = Object.GetMutableAllocationData().
                                                                                 TextureImageAllocations.back().View,
                                                                                 .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR
                                                                         });
        }

        if (Material.normalTexture.index >= 0)
        {
            Output.emplace(AllocateTexture(CommandBuffer,
                                           std::data(Model.images.at(Model.textures.at(Material.normalTexture.index).source).image),
                                           Model.images.at(Model.textures.at(Material.normalTexture.index).source).width,
                                           Model.images.at(Model.textures.at(Material.normalTexture.index).source).height,
                                           GetSwapChainImageFormat(),
                                           std::size(Model.images.at(Model.textures.at(Material.normalTexture.index).source).image),
                                           Object.GetMutableAllocationData().TextureImageAllocations.emplace_back()));

            Object.GetMutableAllocationData().TextureDescriptors.emplace(TextureType::Normal,
                                                                         VkDescriptorImageInfo {
                                                                                 .sampler = GetSampler(),
                                                                                 .imageView = Object.GetMutableAllocationData().
                                                                                 TextureImageAllocations.back().View,
                                                                                 .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR
                                                                         });
        }

        if (Material.occlusionTexture.index >= 0)
        {
            Output.emplace(AllocateTexture(CommandBuffer,
                                           std::data(Model.images.at(Model.textures.at(Material.occlusionTexture.index).source).image),
                                           Model.images.at(Model.textures.at(Material.occlusionTexture.index).source).width,
                                           Model.images.at(Model.textures.at(Material.occlusionTexture.index).source).height,
                                           GetSwapChainImageFormat(),
                                           std::size(Model.images.at(Model.textures.at(Material.occlusionTexture.index).source).image),
                                           Object.GetMutableAllocationData().TextureImageAllocations.emplace_back()));

            Object.GetMutableAllocationData().TextureDescriptors.emplace(TextureType::Occlusion,
                                                                         VkDescriptorImageInfo {
                                                                                 .sampler = GetSampler(),
                                                                                 .imageView = Object.GetMutableAllocationData().
                                                                                 TextureImageAllocations.back().View,
                                                                                 .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR
                                                                         });
        }

        if (Material.emissiveTexture.index >= 0)
        {
            Output.emplace(AllocateTexture(CommandBuffer,
                                           std::data(Model.images.at(Model.textures.at(Material.emissiveTexture.index).source).image),
                                           Model.images.at(Model.textures.at(Material.emissiveTexture.index).source).width,
                                           Model.images.at(Model.textures.at(Material.emissiveTexture.index).source).height,
                                           GetSwapChainImageFormat(),
                                           std::size(Model.images.at(Model.textures.at(Material.emissiveTexture.index).source).image),
                                           Object.GetMutableAllocationData().TextureImageAllocations.emplace_back()));

            Object.GetMutableAllocationData().TextureDescriptors.emplace(TextureType::Emissive,
                                                                         VkDescriptorImageInfo {
                                                                                 .sampler = GetSampler(),
                                                                                 .imageView = Object.GetMutableAllocationData().
                                                                                 TextureImageAllocations.back().View,
                                                                                 .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR
                                                                         });
        }

        if (Material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
        {
            Output.emplace(AllocateTexture(CommandBuffer,
                                           std::data(Model.images.at(Model.textures.at(Material.pbrMetallicRoughness.metallicRoughnessTexture.index).
                                                                           source).image),
                                           Model.images.at(Model.textures.at(Material.pbrMetallicRoughness.metallicRoughnessTexture.index).source).
                                                 width,
                                           Model.images.at(Model.textures.at(Material.pbrMetallicRoughness.metallicRoughnessTexture.index).source).
                                                 height,
                                           GetSwapChainImageFormat(),
                                           std::size(Model.images.at(Model.textures.at(Material.pbrMetallicRoughness.metallicRoughnessTexture.index).
                                                                           source).image),
                                           Object.GetMutableAllocationData().TextureImageAllocations.emplace_back()));

            Object.GetMutableAllocationData().TextureDescriptors.emplace(TextureType::MetallicRoughness,
                                                                         VkDescriptorImageInfo {
                                                                                 .sampler = GetSampler(),
                                                                                 .imageView = Object.GetMutableAllocationData().
                                                                                 TextureImageAllocations.back().View,
                                                                                 .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR
                                                                         });
        }
    }

    for (std::uint8_t TextTypeIter = 0U; TextTypeIter <= static_cast<std::uint8_t>(TextureType::MetallicRoughness); ++TextTypeIter)
    {
        if (!Object.GetMutableAllocationData().TextureDescriptors.contains(static_cast<TextureType>(TextTypeIter)))
        {
            Object.GetMutableAllocationData().TextureDescriptors.emplace(static_cast<TextureType>(TextTypeIter),
                                                                         VkDescriptorImageInfo {
                                                                                 .sampler = GetSampler(),
                                                                                 .imageView = GetEmptyImage().View,
                                                                                 .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR
                                                                         });
        }
    }

    return Output;
}
