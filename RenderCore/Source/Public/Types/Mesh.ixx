// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Types.Mesh;

export import RenderCore.Types.Resource;
export import RenderCore.Types.Transform;
export import RenderCore.Types.Vertex;
export import RenderCore.Types.Material;
export import RenderCore.Types.Texture;

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

        Mesh(std::uint32_t, strzilla::string_view);
        Mesh(std::uint32_t, strzilla::string_view, strzilla::string_view);

        void Optimize();
        void SetupBounds();

        [[nodiscard]] inline Transform const &GetTransform() const
        {
            return m_Transform;
        }

        inline void SetTransform(Transform const &Transform)
        {
            m_Transform = Transform;
        }

        [[nodiscard]] inline glm::vec3 GetCenter() const
        {
            return (m_Bounds.Min + m_Bounds.Max) / 2.0f;
        }

        [[nodiscard]] inline float GetSize() const
        {
            return distance(m_Bounds.Min, m_Bounds.Max);
        }

        [[nodiscard]] inline Bounds const &GetBounds() const
        {
            return m_Bounds;
        }

        [[nodiscard]] inline std::vector<Vertex> const &GetVertices() const
        {
            return m_Vertices;
        }

        inline void SetVertices(std::vector<Vertex> const &Vertices)
        {
            m_Vertices = Vertices;
        }

        [[nodiscard]] inline std::vector<std::uint32_t> const &GetIndices() const
        {
            return m_Indices;
        }

        inline void SetIndices(std::vector<std::uint32_t> const &Indices)
        {
            m_Indices      = Indices;
            m_NumTriangles = static_cast<std::uint32_t>(std::size(m_Indices) / 3U);
        }

        [[nodiscard]] inline std::uint32_t GetNumTriangles() const
        {
            return m_NumTriangles;
        }

        [[nodiscard]] inline std::uint32_t GetNumVertices() const
        {
            return static_cast<std::uint32_t>(std::size(m_Vertices));
        }

        [[nodiscard]] inline std::uint32_t GetNumIndices() const
        {
            return static_cast<std::uint32_t>(std::size(m_Indices));
        }

        [[nodiscard]] inline VkDeviceSize GetVertexOffset() const
        {
            return m_VertexOffset;
        }

        inline void SetVertexOffset(VkDeviceSize const &VertexOffset)
        {
            m_VertexOffset = VertexOffset;
        }

        [[nodiscard]] inline VkDeviceSize GetIndexOffset() const
        {
            return m_IndexOffset;
        }

        inline void SetIndexOffset(VkDeviceSize const &IndexOffset)
        {
            m_IndexOffset = IndexOffset;
        }

        [[nodiscard]] inline MaterialData const &GetMaterialData() const
        {
            return m_MaterialData;
        }

        inline void SetMaterialData(MaterialData const &MaterialData)
        {
            m_MaterialData = MaterialData;
        }

        [[nodiscard]] inline std::vector<std::shared_ptr<Texture>> const &GetTextures() const
        {
            return m_Textures;
        }

        inline void SetTextures(std::vector<std::shared_ptr<Texture>> const &Textures)
        {
            m_Textures = Textures;

            for (auto const &Texture : m_Textures)
            {
                Texture->SetupTexture();
            }
        }

        void BindBuffers(VkCommandBuffer const &, std::uint32_t) const;
    };
} // namespace RenderCore
