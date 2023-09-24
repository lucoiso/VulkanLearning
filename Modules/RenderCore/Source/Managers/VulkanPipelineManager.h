// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "RenderCoreModule.h"
#include "Types/TextureData.h"
#include <vector>
#include <volk.h>

namespace RenderCore
{
    class VulkanPipelineManager
    {
    public:
        VulkanPipelineManager(const VulkanPipelineManager &) = delete;
        VulkanPipelineManager &operator=(const VulkanPipelineManager &) = delete;

        VulkanPipelineManager();
        ~VulkanPipelineManager();

        static VulkanPipelineManager &Get();

        void CreateRenderPass();
        void CreateDefaultRenderPass();
        void CreateGraphicsPipeline(const std::vector<VkPipelineShaderStageCreateInfo> &ShaderStages);
        void CreateDescriptorSetLayout();
        void CreateDescriptorPool();
        void CreateDescriptorSets(const std::vector<VulkanTextureData> &TextureDatas);
        
        static void CreateDescriptorSets(const VkDescriptorPool &DescriptorPool, const VkDescriptorSetLayout &DescriptorSetLayout, const std::vector<VulkanTextureData> &TextureDatas, std::vector<VkDescriptorSet> &DescriptorSets);

        void Shutdown();
        void DestroyResources();
        
        [[nodiscard]] const VkRenderPass &GetRenderPass() const;
        [[nodiscard]] const VkPipeline &GetPipeline() const;
        [[nodiscard]] const VkPipelineLayout &GetPipelineLayout() const;
        [[nodiscard]] const VkPipelineCache &GetPipelineCache() const;
        [[nodiscard]] const VkDescriptorSetLayout &GetDescriptorSetLayout() const;
        [[nodiscard]] const VkDescriptorPool &GetDescriptorPool() const;
        [[nodiscard]] const std::vector<VkDescriptorSet> &GetDescriptorSets() const;

    private:
        static VulkanPipelineManager g_Instance;

        VkRenderPass m_RenderPass;
        VkPipeline m_Pipeline;
        VkPipelineLayout m_PipelineLayout;
        VkPipelineCache m_PipelineCache;
        VkDescriptorPool m_DescriptorPool;
        VkDescriptorSetLayout m_DescriptorSetLayout;
        std::vector<VkDescriptorSet> m_DescriptorSets;
    };
}
