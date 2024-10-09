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
        std::shared_ptr<Mesh> m_Mesh { nullptr };

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
            return m_Mesh->GetTransform();
        }

        inline void SetTransform(Transform const &Value)
        {
            m_Mesh->SetTransform(Value);
        }

        [[nodiscard]] inline glm::vec3 GetPosition() const
        {
            return m_Mesh->GetPosition();
        }

        inline void SetPosition(glm::vec3 const &Value)
        {
            m_Mesh->SetPosition(Value);
        }

        [[nodiscard]] inline glm::vec3 GetRotation() const
        {
            return m_Mesh->GetRotation();
        }

        inline void SetRotation(glm::vec3 const &Value)
        {
            m_Mesh->SetRotation(Value);
        }

        [[nodiscard]] inline glm::vec3 GetScale() const
        {
            return m_Mesh->GetScale();
        }

        inline void SetScale(glm::vec3 const &Value)
        {
            m_Mesh->SetScale(Value);
        }

        [[nodiscard]] inline glm::mat4 GetMatrix() const
        {
            return m_Mesh->GetMatrix();
        }

        inline void SetMatrix(glm::mat4 const &Value)
        {
            m_Mesh->SetMatrix(Value);
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

        void DrawObject(VkCommandBuffer const &, VkPipelineLayout const &, std::uint32_t) const;
    };
} // namespace RenderCore
