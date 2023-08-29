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

        void CreateRenderPass(const VkFormat& Format);
        void CreateGraphicsPipeline(const std::vector<VkPipelineShaderStageCreateInfo>& ShaderStages);

        void Shutdown();

        bool IsInitialized() const;

    private:
        const VkInstance& m_Instance;
        const VkDevice& m_Device;
        VkRenderPass m_RenderPass;
        VkPipeline m_Pipeline;
        VkPipelineLayout m_PipelineLayout;
        VkPipelineCache m_PipelineCache;
        VkDescriptorSetLayout m_DescriptorSetLayout;
    };
}
