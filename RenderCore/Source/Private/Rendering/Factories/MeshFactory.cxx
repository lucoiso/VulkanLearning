// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.Factories.Mesh;

import RenderCore.Runtime.Model;
import RenderCore.Types.Transform;
import RenderCore.Types.Vertex;

using namespace RenderCore;

std::shared_ptr<Mesh> RenderCore::ConstructMesh(MeshConstructionInputParameters const &Arguments)
{
    if (Arguments.Primitive.material < 0)
    {
        return nullptr;
    }

    strzilla::string const MeshName = std::format("{}_{:03d}", std::empty(Arguments.Mesh.name) ? "None" : Arguments.Mesh.name, Arguments.ID);
    auto              NewMesh  = std::make_shared<Mesh>(Arguments.ID, Arguments.Path, MeshName);

    Transform TransformConfig = GetPrimitiveTransform(Arguments.Node);
    NewMesh->SetTransform(std::move(TransformConfig));

    std::vector<Vertex> Vertices = SetVertexAttributes(Arguments.Model, Arguments.Primitive);
    std::vector<std::uint32_t> Indices = AllocatePrimitiveIndices(Arguments.Model, Arguments.Primitive);
    NewMesh->SetupMeshlets(std::move(Vertices), std::move(Indices));

    tinygltf::Material const &MeshMaterial = Arguments.Model.materials.at(Arguments.Primitive.material);
    AlphaMode const AlphaMode = MeshMaterial.alphaMode == "OPAQUE"
                                ? AlphaMode::ALPHA_OPAQUE
                                : MeshMaterial.alphaMode == "MASK"
                                    ? AlphaMode::ALPHA_MASK
                                    : AlphaMode::ALPHA_BLEND;

    NewMesh->SetMaterialData({.AlphaMode = static_cast<std::uint8_t>(AlphaMode),
                              .DoubleSided = static_cast<std::uint8_t>(MeshMaterial.doubleSided),
                              .MetallicFactor = static_cast<float>(MeshMaterial.pbrMetallicRoughness.metallicFactor),
                              .RoughnessFactor = static_cast<float>(MeshMaterial.pbrMetallicRoughness.roughnessFactor),
                              .AlphaCutoff = static_cast<float>(MeshMaterial.alphaCutoff),
                              .NormalScale = static_cast<float>(MeshMaterial.normalTexture.scale),
                              .OcclusionStrength = static_cast<float>(MeshMaterial.occlusionTexture.strength),
                              .EmissiveFactor = glm::make_vec3(std::data(MeshMaterial.emissiveFactor)),
                              .BaseColorFactor = glm::make_vec4(std::data(MeshMaterial.pbrMetallicRoughness.baseColorFactor))});

    std::vector<std::shared_ptr<Texture>> Textures {};

    if (MeshMaterial.pbrMetallicRoughness.baseColorTexture.index >= 0)
    {
        auto const Texture = Arguments.TextureMap.at(MeshMaterial.pbrMetallicRoughness.baseColorTexture.index);
        Texture->AppendType(TextureType::BaseColor);
        Textures.push_back(Texture);
    }

    if (MeshMaterial.normalTexture.index >= 0)
    {
        auto const Texture = Arguments.TextureMap.at(MeshMaterial.normalTexture.index);
        Texture->AppendType(TextureType::Normal);
        Textures.push_back(Texture);
    }

    if (MeshMaterial.occlusionTexture.index >= 0)
    {
        auto const Texture = Arguments.TextureMap.at(MeshMaterial.occlusionTexture.index);
        Texture->AppendType(TextureType::Occlusion);
        Textures.push_back(Texture);
    }

    if (MeshMaterial.emissiveTexture.index >= 0)
    {
        auto const Texture = Arguments.TextureMap.at(MeshMaterial.emissiveTexture.index);
        Texture->AppendType(TextureType::Emissive);
        Textures.push_back(Texture);
    }

    if (MeshMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
    {
        auto const Texture = Arguments.TextureMap.at(MeshMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index);
        Texture->AppendType(TextureType::MetallicRoughness);
        Textures.push_back(Texture);
    }

    NewMesh->SetTextures(std::move(Textures));
    return NewMesh;
}
