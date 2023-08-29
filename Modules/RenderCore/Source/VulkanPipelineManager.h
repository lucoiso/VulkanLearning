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
        VulkanPipelineManager() = delete;
        VulkanPipelineManager(const VulkanPipelineManager&) = delete;
        VulkanPipelineManager& operator=(const VulkanPipelineManager&) = delete;

        VulkanPipelineManager(const VkInstance& Instance, const VkDevice& Device);
        ~VulkanPipelineManager();

        void Initialize(const VkRenderPass& RenderPass, const VkExtent2D& Extent);
        void Shutdown();

        bool IsInitialized() const;
        [[nodiscard]] const std::vector<VkPipeline>& GetPipelines() const;

    private:
        const VkInstance& m_Instance;
        const VkDevice& m_Device;

        std::vector<VkPipeline> m_Pipelines;
        std::unique_ptr<class VulkanShaderCompiler> m_ShaderCompiler;
    };
}
