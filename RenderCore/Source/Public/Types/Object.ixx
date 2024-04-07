// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <Volk/volk.h>
#include <glm/ext.hpp>
#include <string>
#include <vector>
#include <bitset>

#include "RenderCoreModule.hpp"

export module RenderCore.Types.Object;

import RenderCore.Types.Transform;
import RenderCore.Types.Vertex;
import RenderCore.Types.Allocation;
import RenderCore.Types.Material;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Object
    {
        bool                       m_IsPendingDestroy { false };
        mutable bool               m_IsModelRenderDirty { true };
        mutable bool               m_IsMaterialRenderDirty { true };
        std::uint32_t              m_ID {};
        std::string                m_Path {};
        std::string                m_Name {};
        Transform                  m_Transform {};
        std::vector<Vertex>        m_Vertices {};
        std::vector<std::uint32_t> m_Indices {};
        MaterialData               m_MaterialData {};
        ObjectAllocationData       m_Allocation {};

    public:
        Object()          = delete;
        virtual ~Object() = default;

        inline bool operator==(Object const &Rhs) const
        {
            return GetID() == Rhs.GetID() && GetName() == Rhs.GetName() && GetPath() == Rhs.GetPath();
        }

        Object(std::uint32_t, std::string_view);
        Object(std::uint32_t, std::string_view, std::string_view);

        [[nodiscard]] std::uint32_t      GetID() const;
        [[nodiscard]] std::string const &GetPath() const;
        [[nodiscard]] std::string const &GetName() const;

        [[nodiscard]] Transform const &GetTransform() const;
        void                           SetTransform(Transform const &);

        [[nodiscard]] glm::vec3 GetPosition() const;
        void                    SetPosition(glm::vec3 const &);

        [[nodiscard]] glm::vec3 GetRotation() const;
        void                    SetRotation(glm::vec3 const &);

        [[nodiscard]] glm::vec3 GetScale() const;
        void                    SetScale(glm::vec3 const &);

        [[nodiscard]] glm::mat4 GetMatrix() const;
        void                    SetMatrix(glm::mat4 const &);

        [[nodiscard]] MaterialData const &GetMaterialData() const;
        void                              SetMaterialData(MaterialData const &);

        virtual void Tick(double)
        {
        }

        [[nodiscard]] bool IsPendingDestroy() const;
        void               Destroy();

        [[nodiscard]] ObjectAllocationData const &GetAllocationData() const;
        [[nodiscard]] ObjectAllocationData &      GetMutableAllocationData();

        void                                     SetVertexBuffer(std::vector<Vertex> const &);
        [[nodiscard]] std::vector<Vertex> const &GetVertices() const;

        void                                            SetIndexBuffer(std::vector<std::uint32_t> const &);
        [[nodiscard]] std::vector<std::uint32_t> const &GetIndices() const;

        [[nodiscard]] std::uint32_t GetNumTriangles() const;

        void UpdateUniformBuffers() const;
        void DrawObject(VkCommandBuffer const &) const;
    };
} // namespace RenderCore
