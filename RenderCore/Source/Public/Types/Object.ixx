// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <Volk/volk.h>
#include <glm/ext.hpp>
#include <string>
#include <vector>

#include "RenderCoreModule.hpp"

export module RenderCore.Types.Object;

import RenderCore.Types.Transform;
import RenderCore.Types.Vertex;
import RenderCore.Types.Allocation;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Object
    {
        bool                       m_IsPendingDestroy {false};
        std::uint32_t              m_ID {};
        std::string                m_Path {};
        std::string                m_Name {};
        Transform                  m_Transform {};
        ObjectAllocationData       m_Allocation {};
        std::vector<Vertex>        m_Vertices {};
        std::vector<std::uint32_t> m_Indices {};

    public:
                 Object() = delete;
        virtual ~Object() = default;

        Object(std::uint32_t, std::string_view);
        Object(std::uint32_t, std::string_view, std::string_view);

        [[nodiscard]] std::uint32_t      GetID() const;
        [[nodiscard]] std::string const &GetPath() const;
        [[nodiscard]] std::string const &GetName() const;

        [[nodiscard]] Transform const &GetTransform() const;
        [[nodiscard]] Transform       &GetMutableTransform();
        void                           SetTransform(Transform const &Value);

        [[nodiscard]] Vector GetPosition() const;
        void                 SetPosition(Vector const &Position);

        [[nodiscard]] Rotator GetRotation() const;
        void                  SetRotation(Rotator const &Rotation);

        [[nodiscard]] Vector GetScale() const;
        void                 SetScale(Vector const &Scale);

        [[nodiscard]] glm::mat4 GetMatrix() const;

        virtual void Tick(double)
        {
        }

        [[nodiscard]] bool IsPendingDestroy() const;
        void               Destroy();

        [[nodiscard]] ObjectAllocationData const &GetAllocationData() const;
        [[nodiscard]] ObjectAllocationData       &GetMutableAllocationData();

        void                                     SetVertexBuffer(std::vector<Vertex> const &);
        [[nodiscard]] std::vector<Vertex> const &GetVertices() const;
        [[nodiscard]] std::vector<Vertex>       &GetMutableVertices();

        void                                            SetIndexBuffer(std::vector<std::uint32_t> const &);
        [[nodiscard]] std::vector<std::uint32_t> const &GetIndices() const;
        [[nodiscard]] std::vector<std::uint32_t>       &GetMutableIndices();

        [[nodiscard]] std::uint32_t GetNumTriangles() const;

        void UpdateUniformBuffers() const;
        void DrawObject(VkCommandBuffer const &) const;
    };
} // namespace RenderCore
