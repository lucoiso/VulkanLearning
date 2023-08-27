// Copyright Notice: [...]

#pragma once

#include "RenderCoreModule.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace RenderCore
{
class VulkanPipelineManager
{
    public:
        VulkanPipelineManager();

        VulkanPipelineManager(const VulkanPipelineManager&) = delete;
        VulkanPipelineManager& operator=(const VulkanPipelineManager&) = delete;

        ~VulkanPipelineManager();

        void Initialize(const VkDevice& Device, const VkRenderPass& RenderPass, const VkExtent2D& Extent);
        void Shutdown();

        // Testing only - will be removed
        void CompileShader(const char* ShaderSource);

        bool IsInitialized() const;

    private:
        std::vector<VkPipeline> m_Pipelines;
        VkDevice m_Device;
        std::unique_ptr<class VulkanShaderCompiler> m_ShaderCompiler;
    };
}
