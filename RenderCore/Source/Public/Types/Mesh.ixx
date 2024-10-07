// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Types.Mesh;

import RenderCore.Types.Resource;
import RenderCore.Types.Transform;
import RenderCore.Types.Vertex;
import RenderCore.Types.Material;
import RenderCore.Types.Texture;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Mesh : public Resource
    {
        Transform m_Transform {};

        std::vector<Meshlet> m_Meshlets {};
        VkDeviceSize m_MeshletOffset { 0U };

        MaterialData                          m_MaterialData {};
        std::vector<std::shared_ptr<Texture>> m_Textures {};

    public:
        ~Mesh() override = default;
        Mesh()           = delete;

        Mesh(std::uint32_t, strzilla::string_view);
        Mesh(std::uint32_t, strzilla::string_view, strzilla::string_view);

        [[nodiscard]] inline Transform const &GetTransform() const
        {
            return m_Transform;
        }

        inline void SetTransform(Transform const &Value)
        {
            if (m_Transform != Value)
            {
                m_Transform = Value;
                MarkAsRenderDirty();
            }
        }

        [[nodiscard]] inline std::vector<Meshlet> const &GetMeshlets() const
        {
            return m_Meshlets;
        }

        inline void SetMeshlets(std::vector<Meshlet> const &Value)
        {
            m_Meshlets = Value;
            MarkAsRenderDirty();
        }

        [[nodiscard]] inline std::uint32_t GetNumMeshlets() const
        {
            return static_cast<std::uint32_t>(std::size(m_Meshlets));
        }

        [[nodiscard]] inline VkDeviceSize GetMeshletOffset() const
        {
            return m_MeshletOffset;
        }

        inline void SetMeshletOffset(VkDeviceSize const &Value)
        {
            m_MeshletOffset = Value;
            MarkAsRenderDirty();
        }

        void SetupVertices(std::vector<Vertex> const&);
        void SetupIndices(std::vector<std::uint32_t> const&);

        [[nodiscard]] inline MaterialData const &GetMaterialData() const
        {
            return m_MaterialData;
        }

        inline void SetMaterialData(MaterialData const &Value)
        {
            if (m_MaterialData != Value)
            {
                m_MaterialData = Value;
                MarkAsRenderDirty();
            }
        }

        [[nodiscard]] inline std::vector<std::shared_ptr<Texture>> const &GetTextures() const
        {
            return m_Textures;
        }

        inline void SetTextures(std::vector<std::shared_ptr<Texture>> const &Value)
        {
            if (m_Textures != Value)
            {
                m_Textures = Value;

                for (auto const &Texture : m_Textures)
                {
                    Texture->SetupTexture();
                }

                MarkAsRenderDirty();
            }
        }

        void UpdateUniformBuffers(char *) const;
    };
} // namespace RenderCore
