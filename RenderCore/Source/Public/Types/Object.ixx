// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <Volk/volk.h>
#include <glm/ext.hpp>

export module RenderCore.Types.Object;

import RenderCore.Types.Transform;
import RenderCore.Types.Resource;
import RenderCore.Types.Mesh;

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

        [[nodiscard]] Transform const &GetTransform() const;
        void                           SetTransform(Transform const &);

        [[nodiscard]] std::uint32_t GetNumInstances() const;
        void                        SetNumInstance(std::uint32_t);

        [[nodiscard]] Transform const &GetInstanceTransform(std::uint32_t) const;
        void                           SetInstanceTransform(std::uint32_t, Transform const &);

        [[nodiscard]] glm::vec3 GetPosition() const;
        void                    SetPosition(glm::vec3 const &);

        [[nodiscard]] glm::vec3 GetRotation() const;
        void                    SetRotation(glm::vec3 const &);

        [[nodiscard]] glm::vec3 GetScale() const;
        void                    SetScale(glm::vec3 const &);

        [[nodiscard]] glm::mat4 GetMatrix() const;
        void                    SetMatrix(glm::mat4 const &);

        virtual void Tick(double)
        {
        }

        void Destroy() override;

        [[nodiscard]] std::uint32_t GetUniformOffset() const;
        void                        SetUniformOffset(std::uint32_t const &);

        void SetupUniformDescriptor();

        void UpdateUniformBuffers() const;
        void DrawObject(VkCommandBuffer const &, VkPipelineLayout const &, std::uint32_t) const;

        [[nodiscard]] std::shared_ptr<Mesh> GetMesh() const;
        void                                SetMesh(std::shared_ptr<Mesh> const &);

        [[nodiscard]] bool IsRenderDirty() const;
        void MarkAsRenderDirty() const;
    };
} // namespace RenderCore
