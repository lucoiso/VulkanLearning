// Copyright Notice: [...]

#pragma once

#include "RenderCoreModule.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace RenderCore
{
class VulkanPipeline
{
    public:
        VulkanPipeline();

        VulkanPipeline(const VulkanPipeline&) = delete;
        VulkanPipeline& operator=(const VulkanPipeline&) = delete;

        ~VulkanPipeline();

        void Initialize(const VkDevice& device, const VkRenderPass& renderPass, const VkExtent2D& extent);
        void Shutdown();

        bool IsInitialized() const;

    private:
        std::vector<VkPipeline> m_Pipelines;
        VkDevice m_Device;
        std::unique_ptr<class VulkanShaderCompiler> m_ShaderCompiler;
    };
}
