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
        mutable bool m_IsRenderDirty{ true };

        Transform              m_Transform{};
        std::vector<Transform> m_InstanceTransform{};

        VkDeviceSize m_ModelOffset{ 0U };
        VkDeviceSize m_MaterialOffset{ 0U };
        VkDeviceSize m_MeshletsOffset{ 0U };
        VkDeviceSize m_IndicesOffset{ 0U };
        VkDeviceSize m_VerticesOffset{ 0U };

        MaterialData                          m_MaterialData {};
        std::vector<Meshlet>                  m_Meshlets {};
        std::vector<glm::uint>                m_Indices {};
        std::vector<Vertex>                   m_Vertices {};
        std::vector<std::shared_ptr<Texture>> m_Textures {};

    public:
        ~Mesh() override = default;
        Mesh()           = delete;

        Mesh(std::uint32_t, strzilla::string_view);
        Mesh(std::uint32_t, strzilla::string_view, strzilla::string_view);

        [[nodiscard]] inline Transform const& GetTransform() const
        {
            return m_Transform;
        }

        inline void SetTransform(Transform const& Value)
        {
            if (m_Transform != Value)
            {
                m_Transform = Value;
                MarkAsRenderDirty();
            }
        }

        [[nodiscard]] inline std::size_t GetNumInstances() const
        {
            return std::size(m_InstanceTransform);
        }

        inline void SetNumInstance(std::size_t const Value)
        {
            if (GetNumInstances() != Value)
            {
                m_InstanceTransform.resize(Value);
                MarkAsRenderDirty();
            }
        }

        [[nodiscard]] inline Transform const& GetInstanceTransform(std::uint32_t const Index) const
        {
            return m_InstanceTransform.at(Index);
        }

        inline void SetInstanceTransform(std::uint32_t const Index, Transform const& Value)
        {
            if (Transform& TransformIt = m_InstanceTransform.at(Index);
                TransformIt != Value)
            {
                TransformIt = Value;
                MarkAsRenderDirty();
            }
        }

        [[nodiscard]] inline glm::vec3 GetPosition() const
        {
            return m_Transform.GetPosition();
        }

        inline void SetPosition(glm::vec3 const& Value)
        {
            if (m_Transform.GetPosition() != Value)
            {
                m_Transform.SetPosition(Value);
                MarkAsRenderDirty();
            }
        }

        [[nodiscard]] inline glm::vec3 GetRotation() const
        {
            return m_Transform.GetRotation();
        }

        inline void SetRotation(glm::vec3 const& Value)
        {
            if (m_Transform.GetRotation() != Value)
            {
                m_Transform.SetRotation(Value);
                MarkAsRenderDirty();
            }
        }

        [[nodiscard]] inline glm::vec3 GetScale() const
        {
            return m_Transform.GetScale();
        }

        inline void SetScale(glm::vec3 const& Value)
        {
            if (m_Transform.GetScale() != Value)
            {
                m_Transform.SetScale(Value);
                MarkAsRenderDirty();
            }
        }

        [[nodiscard]] inline glm::mat4 GetMatrix() const
        {
            return m_Transform.GetMatrix();
        }

        inline void SetMatrix(glm::mat4 const& Value)
        {
            m_Transform.SetMatrix(Value);
            MarkAsRenderDirty();
        }

        [[nodiscard]] inline VkDeviceSize GetModelOffset() const
        {
            return m_ModelOffset;
        }

        inline void SetModelOffset(VkDeviceSize const& Offset)
        {
            m_ModelOffset = Offset;
        }

        [[nodiscard]] inline VkDeviceSize GetMaterialOffset() const
        {
            return m_MaterialOffset;
        }

        inline void SetMaterialOffset(VkDeviceSize const& Offset)
        {
            m_MaterialOffset = Offset;
        }

        [[nodiscard]] inline std::vector<Meshlet> const &GetMeshlets() const
        {
            return m_Meshlets;
        }

        [[nodiscard]] inline std::size_t GetNumMeshlets() const
        {
            return std::size(m_Meshlets);
        }

        [[nodiscard]] inline VkDeviceSize GetMeshletsOffset() const
        {
            return m_MeshletsOffset;
        }

        inline void SetMeshletsOffset(VkDeviceSize const &Value)
        {
            m_MeshletsOffset = Value;
        }

        [[nodiscard]] inline VkDeviceSize GetIndicesOffset() const
        {
            return m_IndicesOffset;
        }

        inline void SetIndicesOffset(VkDeviceSize const& Value)
        {
            m_IndicesOffset = Value;
        }

        [[nodiscard]] inline VkDeviceSize GetVerticesOffset() const
        {
            return m_VerticesOffset;
        }

        inline void SetVerticesOffset(VkDeviceSize const& Value)
        {
            m_VerticesOffset = Value;
        }

        [[nodiscard]] inline std::vector<glm::uint> const& GetIndices() const
        {
            return m_Indices;
        }

        [[nodiscard]] inline std::size_t GetNumIndices() const
        {
            return std::size(m_Indices);
        }

        [[nodiscard]] inline std::vector<Vertex> const& GetVertices() const
        {
            return m_Vertices;
        }

        [[nodiscard]] inline std::size_t GetNumVertices() const
        {
            return std::size(m_Vertices);
        }

        void SetupMeshlets(std::vector<Vertex> &&, std::vector<std::uint32_t> &&);

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

        void SetupUniformDescriptor() const;

        void UpdateUniformBuffers() const;
        void UpdatePrimitivesBuffers() const;

        void DrawObject(VkCommandBuffer const&, VkPipelineLayout const&, std::uint32_t) const;
    };
} // namespace RenderCore
