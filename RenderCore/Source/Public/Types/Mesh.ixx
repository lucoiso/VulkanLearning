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

#define DECLARE_VECTOR_PROPERTY(Property, Type) \
Type m_##Property {}; \
VkDeviceSize m_##Property##Offset{ 0U }; \
VkDeviceSize m_##Property##LocalOffset{ 0U };

#define GET_SET_VECTOR_PROPERTY(Property, Type) \
[[nodiscard]] inline std::uint32_t GetNum##Property() const \
{ \
    return static_cast<std::uint32_t>(std::size(m_##Property)); \
} \
[[nodiscard]] inline VkDeviceSize Get##Property##Offset() const \
{ \
    return m_##Property##Offset; \
} \
inline void Set##Property##Offset(VkDeviceSize const& Value) \
{ \
    m_##Property##Offset = Value; \
} \
[[nodiscard]] inline VkDeviceSize Get##Property##LocalOffset() const \
{ \
    return m_##Property##LocalOffset; \
} \
inline void Set##Property##LocalOffset(VkDeviceSize const& Value) \
{ \
    m_##Property##LocalOffset = Value; \
} \
[[nodiscard]] inline Type const& Get##Property() const \
{ \
    return m_##Property; \
}

namespace RenderCore
{
    export class RENDERCOREMODULE_API Mesh : public Resource
    {
        Transform m_Transform {};

        MaterialData                          m_MaterialData{};
        std::vector<std::shared_ptr<Texture>> m_Textures{};

        DECLARE_VECTOR_PROPERTY(Meshlets,        std::vector<Meshlet>)
        DECLARE_VECTOR_PROPERTY(Indices,         std::vector<std::uint32_t>)
        DECLARE_VECTOR_PROPERTY(VertexUVs,       std::vector<glm::vec2>)
        DECLARE_VECTOR_PROPERTY(VertexPositions, std::vector<glm::vec3>)
        DECLARE_VECTOR_PROPERTY(VertexNormals,   std::vector<glm::vec3>)
        DECLARE_VECTOR_PROPERTY(VertexColors,    std::vector<glm::vec4>)
        DECLARE_VECTOR_PROPERTY(VertexJoints,    std::vector<glm::vec4>)
        DECLARE_VECTOR_PROPERTY(VertexWeights,   std::vector<glm::vec4>)
        DECLARE_VECTOR_PROPERTY(VertexTangents,  std::vector<glm::vec4>)

        VkDeviceSize m_MaterialOffset{ 0U };
        VkDeviceSize m_UniformOffset{ 0U };

        VkDescriptorBufferInfo m_UniformBufferInfo{};
        void* m_MappedData{ nullptr };

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

        [[nodiscard]] inline VkDeviceSize GetMaterialOffset() const
        {
            return m_MaterialOffset;
        }

        inline void SetMaterialOffset(VkDeviceSize const& Value)
        {
            m_MaterialOffset = Value;
        }

        [[nodiscard]] inline VkDeviceSize GetUniformOffset() const
        {
            return m_UniformOffset;
        }

        inline void SetUniformOffset(VkDeviceSize const& Offset)
        {
            m_UniformOffset = Offset;
        }

        GET_SET_VECTOR_PROPERTY(Meshlets,        std::vector<Meshlet>      )
        GET_SET_VECTOR_PROPERTY(Indices,         std::vector<std::uint32_t>)
        GET_SET_VECTOR_PROPERTY(VertexUVs,       std::vector<glm::vec2>    )
        GET_SET_VECTOR_PROPERTY(VertexPositions, std::vector<glm::vec3>    )
        GET_SET_VECTOR_PROPERTY(VertexNormals,   std::vector<glm::vec3>    )
        GET_SET_VECTOR_PROPERTY(VertexColors,    std::vector<glm::vec4>    )
        GET_SET_VECTOR_PROPERTY(VertexJoints,    std::vector<glm::vec4>    )
        GET_SET_VECTOR_PROPERTY(VertexWeights,   std::vector<glm::vec4>    )
        GET_SET_VECTOR_PROPERTY(VertexTangents,  std::vector<glm::vec4>    )

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

        void SetupUniformDescriptor();
        void UpdateUniformBuffers() const;

        void DrawObject(VkCommandBuffer const&, VkPipelineLayout const&, std::uint32_t) const;
    };
} // namespace RenderCore

#undef GET_SET_VECTOR_PROPERTY
#undef DECLARE_VECTOR_PROPERTY