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
        void CreateGraphicsPipeline(const std::vector<VkPipelineShaderStageCreateInfo>& ShaderStages, const VkExtent2D& Extent);

        void Shutdown();

        bool IsInitialized() const;
        [[nodiscard]] const VkRenderPass& GetRenderPass() const;
        [[nodiscard]] const VkPipeline& GetPipeline() const;
        [[nodiscard]] const VkPipelineLayout& GetPipelineLayout() const;
        // [[nodiscard]] const VkPipelineCache& GetPipelineCache() const;
        // [[nodiscard]] const VkDescriptorSetLayout& GetDescriptorSetLayout() const;

        void StartRenderPass(const VkExtent2D& Extent, const std::vector<VkCommandBuffer>& CommandBuffer, const std::vector<VkFramebuffer>& FrameBuffers, const std::vector<VkBuffer>& VertexBuffer, const std::vector<VkDeviceSize>& Offsets);

    private:
        const VkInstance& m_Instance;
        const VkDevice& m_Device;

        VkRenderPass m_RenderPass;
        VkPipeline m_Pipeline;
        VkPipelineLayout m_PipelineLayout;
        // VkPipelineCache m_PipelineCache;
        // VkDescriptorSetLayout m_DescriptorSetLayout;
        VkViewport m_Viewport;
        VkRect2D m_Scissor;
    };
}
