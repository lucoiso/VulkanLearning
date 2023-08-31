// Copyright Notice: [...]

#pragma once

#include "RenderCoreModule.h"
#include <memory>

struct GLFWwindow;

namespace RenderCore
{
    class RENDERCOREMODULE_API VulkanRender
    {
        class Impl;

    public:
        VulkanRender();

        VulkanRender(const VulkanRender&) = delete;
        VulkanRender& operator=(const VulkanRender&) = delete;

        ~VulkanRender();

        bool Initialize(GLFWwindow* const Window);
        void Shutdown();

        void DrawFrame(GLFWwindow* const Window);

        bool IsInitialized() const;

    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
