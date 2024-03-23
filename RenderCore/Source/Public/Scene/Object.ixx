// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <string>
#include <glm/ext.hpp>
#include "RenderCoreModule.hpp"

export module RenderCore.Types.Object;

import RenderCore.Types.Transform;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Object
    {
        std::uint32_t m_ID {};
        std::string m_Path {};
        std::string m_Name {};
        std::uint32_t m_TrianglesCount {};
        Transform m_Transform {};
        bool m_IsPendingDestroy {false};

        friend class BufferManager;
        void SetTrianglesCount(std::uint32_t);

    public:
        Object()          = delete;
        virtual ~Object() = default;

        Object(std::uint32_t, std::string_view);
        Object(std::uint32_t, std::string_view, std::string_view);

        [[nodiscard]] std::uint32_t GetID() const;
        [[nodiscard]] std::string const& GetPath() const;
        [[nodiscard]] std::string const& GetName() const;
        [[nodiscard]] std::uint32_t GetTrianglesCount() const;

        [[nodiscard]] Transform const& GetTransform() const;
        [[nodiscard]] Transform& GetMutableTransform();
        void SetTransform(Transform const& Value);

        [[nodiscard]] Vector GetPosition() const;
        void SetPosition(Vector const& Position);

        [[nodiscard]] Rotator GetRotation() const;
        void SetRotation(Rotator const& Rotation);

        [[nodiscard]] Vector GetScale() const;
        void SetScale(Vector const& Scale);

        [[nodiscard]] glm::mat4 GetMatrix() const;

        virtual void Tick(double)
        {
        }

        [[nodiscard]] bool IsPendingDestroy() const;
        void Destroy();
    };
}// namespace RenderCore