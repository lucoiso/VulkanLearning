// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include "RenderCoreModule.h"

#include <GLFW/glfw3.h>

export module RenderCore.Subsystem.Rendering;

import <unordered_map>;
import RenderCore.Renderer;

namespace RenderCore
{
    export class RENDERCOREMODULE_API RenderingSubsystem
    {
        std::unordered_map<GLFWwindow*, Renderer*> m_RegisteredRenderers {};

        RenderingSubsystem()  = default;
        ~RenderingSubsystem() = default;

    public:
        static [[nodiscard]] RenderingSubsystem& Get();

        void RegisterRenderer(GLFWwindow*, Renderer&);
        void UnregisterRenderer(GLFWwindow*);

        [[nodiscard]] Renderer* GetRenderer(GLFWwindow*) const;
    };
}// namespace RenderCore