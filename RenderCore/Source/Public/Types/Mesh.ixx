// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <memory>
#include <string_view>
#include <vector>
#include <glm/ext.hpp>
#include <Volk/volk.h>

#include "RenderCoreModule.hpp"

export module RenderCore.Types.Mesh;

import RenderCore.Types.Transform;
import RenderCore.Types.Vertex;
import RenderCore.Types.Resource;
import RenderCore.Types.Texture;
import RenderCore.Types.Material;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Mesh : public Resource
    {
        Bounds                     m_Bounds {};
        Transform                  m_Transform {};
        std::vector<Vertex>        m_Vertices {};
        std::vector<std::uint32_t> m_Indices {};
        std::uint32_t              m_NumTriangles { 0U };

        VkDeviceSize m_VertexOffset { 0U };
        VkDeviceSize m_IndexOffset { 0U };

        MaterialData                          m_MaterialData {};
        std::vector<std::shared_ptr<Texture>> m_Textures {};

    public:
        ~Mesh() override = default;
        Mesh()           = delete;

        Mesh(std::uint32_t, std::string_view);
        Mesh(std::uint32_t, std::string_view, std::string_view);

        [[nodiscard]] Transform const &GetTransform() const;
        void                           SetTransform(Transform const &Transform);

        [[nodiscard]] glm::vec3     GetCenter() const;
        [[nodiscard]] float         GetSize() const;
        [[nodiscard]] Bounds const &GetBounds() const;
        void                        SetupBounds();

        [[nodiscard]] std::vector<Vertex> const &GetVertices() const;
        void                                     SetVertices(std::vector<Vertex> const &Vertices);

        [[nodiscard]] std::vector<std::uint32_t> const &GetIndices() const;
        void                                            SetIndices(std::vector<std::uint32_t> const &Indices);

        [[nodiscard]] std::uint32_t GetNumTriangles() const;

        [[nodiscard]] VkDeviceSize GetVertexOffset() const;
        void                       SetVertexOffset(VkDeviceSize const &VertexOffset);

        [[nodiscard]] VkDeviceSize GetIndexOffset() const;
        void                       SetIndexOffset(VkDeviceSize const &IndexOffset);

        [[nodiscard]] MaterialData const &GetMaterialData() const;
        void                              SetMaterialData(MaterialData const &MaterialData);

        [[nodiscard]] std::vector<std::shared_ptr<Texture>> const &GetTextures() const;
        void                                                       SetTextures(std::vector<std::shared_ptr<Texture>> const &Textures);

        [[nodiscard]] std::vector<VkWriteDescriptorSet> GetWriteDescriptorSet() const override;
        void                                            BindBuffers(VkCommandBuffer const &) const;
    };
} // namespace RenderCore
