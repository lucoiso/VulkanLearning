// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include "RenderCoreModule.hpp"
#include <glm/ext.hpp>

export module RenderCore.Types.Illumination;

import RenderCore.Types.Transform;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Illumination
    {
        glm::vec3 m_LightPosition{100.F, 100.F, 100.F};
        glm::vec3 m_LightColor{1.F, 1.F, 1.F};
        float     m_LightIntensity{1.F};

    public:
        Illumination() = default;

        void                           SetPosition(glm::vec3 const &);
        [[nodiscard]] glm::vec3 const &GetPosition() const;

        void                           SetColor(glm::vec3 const &);
        [[nodiscard]] glm::vec3 const &GetColor() const;

        void                SetIntensity(float);
        [[nodiscard]] float GetIntensity() const;
    };
} // namespace RenderCore
