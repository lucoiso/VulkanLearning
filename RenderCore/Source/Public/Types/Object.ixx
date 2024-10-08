// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Types.Object;

import RenderCore.Types.Mesh;
import RenderCore.Types.Vertex;
import RenderCore.Types.Resource;
import RenderCore.Types.Transform;
import RenderCore.Types.Material;
import RenderCore.Types.UniformBufferObject;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Object : public Resource
    {
        mutable bool           m_IsRenderDirty { true };
        Transform              m_Transform {};
        std::vector<Transform> m_InstanceTransform {};
        std::shared_ptr<Mesh>  m_Mesh { nullptr };
        std::uint32_t          m_UniformOffset {};
        VkDescriptorBufferInfo m_UniformBufferInfo {};
        void *                 m_MappedData { nullptr };

    public:
        Object()           = delete;
        ~Object() override = default;

        inline bool operator==(Object const &Rhs) const
        {
            return GetID() == Rhs.GetID() && GetName() == Rhs.GetName() && GetPath() == Rhs.GetPath();
        }

        Object(std::uint32_t, strzilla::string_view);
        Object(std::uint32_t, strzilla::string_view, strzilla::string_view);

        [[nodiscard]] inline Transform const &GetTransform() const
        {
            return m_Transform;
        }

        inline void SetTransform(Transform const &Value)
        {
            if (m_Transform != Value)
            {
                m_Transform     = Value;
                MarkAsRenderDirty();
            }
        }

        [[nodiscard]] inline std::uint32_t GetNumInstances() const
        {
            return static_cast<std::uint32_t>(std::size(m_InstanceTransform));
        }

        inline void SetNumInstance(std::uint32_t const Value)
        {
            if (GetNumInstances() != Value)
            {
                m_InstanceTransform.resize(Value);
                MarkAsRenderDirty();
            }
        }

        [[nodiscard]] inline Transform const &GetInstanceTransform(std::uint32_t const Index) const
        {
            return m_InstanceTransform.at(Index);
        }

        inline void SetInstanceTransform(std::uint32_t const Index, Transform const &Value)
        {
            if (Transform &TransformIt = m_InstanceTransform.at(Index);
                TransformIt != Value)
            {
                TransformIt     = Value;
                MarkAsRenderDirty();
            }
        }

        [[nodiscard]] inline glm::vec3 GetPosition() const
        {
            return m_Transform.GetPosition();
        }

        inline void SetPosition(glm::vec3 const &Value)
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

        inline void SetRotation(glm::vec3 const &Value)
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

        inline void SetScale(glm::vec3 const &Value)
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

        inline void SetMatrix(glm::mat4 const &Value)
        {
            m_Transform.SetMatrix(Value);
            MarkAsRenderDirty();
        }

        [[nodiscard]] inline std::uint32_t GetUniformOffset() const
        {
            return m_UniformOffset;
        }

        [[nodiscard]] inline std::uint32_t GetMaterialOffset() const
        {
            return m_UniformOffset + sizeof(ModelUniformData);
        }

        inline void SetUniformOffset(std::uint32_t const &Offset)
        {
            m_UniformOffset = Offset;
        }

        [[nodiscard]] inline std::shared_ptr<Mesh> const &GetMesh() const
        {
            return m_Mesh;
        }

        inline void SetMesh(std::shared_ptr<Mesh> const &Value)
        {
            m_Mesh = Value;
        }

        void Destroy() override;

        virtual void Tick(double)
        {
        }

        void SetupUniformDescriptor();
        void UpdateUniformBuffers() const;
        void DrawObject(VkCommandBuffer const &, VkPipelineLayout const &, std::uint32_t) const;
    };
} // namespace RenderCore
