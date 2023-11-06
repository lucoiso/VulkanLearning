// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include "RenderCoreModule.h"
#include <glm/ext.hpp>

export module RenderCore.Types.Object;

export import <string>;
export import <string_view>;
export import RenderCore.Types.Transform;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Object
    {
        std::uint32_t m_ID {};
        std::string m_Path {};
        std::string m_Name {};
        Transform m_Transform {};

    public:
        Object() = delete;

        Object(Object const&)            = default;
        Object& operator=(Object const&) = default;

        Object(std::uint32_t, std::string_view);
        virtual ~Object();

        [[nodiscard]] std::uint32_t GetID() const;
        [[nodiscard]] std::string_view GetPath() const;
        [[nodiscard]] std::string_view GetName() const;

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

        virtual void Tick(double) {};
        void Destroy();
    };
}// namespace RenderCore