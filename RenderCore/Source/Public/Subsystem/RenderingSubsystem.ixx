// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include "RenderCoreModule.hpp"

export module RenderCore.Subsystem.Rendering;

import RenderCore.Renderer;

namespace RenderCore
{
    export class RENDERCOREMODULE_API RenderingSubsystem
    {
        Renderer *m_RegisteredRenderer {};

         RenderingSubsystem() = default;
        ~RenderingSubsystem() = default;

    public:
        static [[nodiscard]] RenderingSubsystem &Get();

        void RegisterRenderer(Renderer *);
        void UnregisterRenderer();

        [[nodiscard]] Renderer *GetRenderer() const;
    };
} // namespace RenderCore
