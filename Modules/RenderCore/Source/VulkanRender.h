// Copyright Notice: [...]

#pragma once

#include "RenderCoreModule.h"
#include "VulkanConfigurator.h"
#include <memory>

namespace RenderCore
{
    class RENDERCOREMODULE_API VulkanRender
    {
    public:
        VulkanRender();

        VulkanRender(const VulkanRender&) = delete;
        VulkanRender& operator=(const VulkanRender&) = delete;

        ~VulkanRender();

        void Initialize(GLFWwindow* const Window);
        void Shutdown();

        bool IsInitialized() const;

    private:
        std::unique_ptr<VulkanConfigurator> m_Configurator;
    };
}
