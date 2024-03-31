// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include "RenderCoreModule.hpp"

export module RenderCore.Types.Illumination;

import RenderCore.Types.Transform;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Illumination
    {
        Vector m_LightPosition {0.f, 2.5f, 1.f};
        Vector m_LightColor {1.F, 1.F, 1.F};
        float  m_LightIntensity {1.F};

    public:
        Illumination() = default;

        void                        SetPosition(Vector const &);
        [[nodiscard]] Vector const &GetPosition() const;

        void                        SetColor(Vector const &);
        [[nodiscard]] Vector const &GetColor() const;

        void                SetIntensity(float);
        [[nodiscard]] float GetIntensity() const;
    };
} // namespace RenderCore
