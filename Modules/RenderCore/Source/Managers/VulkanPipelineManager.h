// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "RenderCoreModule.h"
#include <vulkan/vulkan_core.h>
#include <memory>
#include <vector>

namespace RenderCore
{
    class VulkanPipelineManager
    {
    public:
        VulkanPipelineManager() = delete;
        VulkanPipelineManager(const VulkanPipelineManager &) = delete;
        VulkanPipelineManager &operator=(const VulkanPipelineManager &) = delete;

        VulkanPipelineManager(const VkInstance &Instance, const VkDevice &Device);
        ~VulkanPipelineManager();

        void CreateRenderPass(const VkFormat &Format);
        void CreateGraphicsPipeline(const std::vector<VkPipelineShaderStageCreateInfo> &ShaderStages);
        void CreateDescriptorSetLayout();
        void CreateDescriptorPool();
        void CreateDescriptorSets(const std::vector<VkBuffer> &UniformBuffers);

        void Shutdown();

        bool IsInitialized() const;
        [[nodiscard]] const VkRenderPass &GetRenderPass() const;
        [[nodiscard]] const VkPipeline &GetPipeline() const;
        [[nodiscard]] const VkPipelineLayout &GetPipelineLayout() const;
        [[nodiscard]] const VkPipelineCache &GetPipelineCache() const;
        [[nodiscard]] const VkDescriptorSetLayout &GetDescriptorSetLayout() const;
        [[nodiscard]] const VkDescriptorPool &GetDescriptorPool() const;
        [[nodiscard]] const std::vector<VkDescriptorSet> &GetDescriptorSets() const;

    private:
        const VkInstance &m_Instance;
        const VkDevice &m_Device;

        VkRenderPass m_RenderPass;
        VkPipeline m_Pipeline;
        VkPipelineLayout m_PipelineLayout;
        VkPipelineCache m_PipelineCache;
        VkDescriptorPool m_DescriptorPool;
        VkDescriptorSetLayout m_DescriptorSetLayout;
        std::vector<VkDescriptorSet> m_DescriptorSets;
    };
}
