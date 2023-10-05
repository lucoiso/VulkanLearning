// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <volk.h>

export module RenderCore.Managers.PipelineManager;

import <vector>;

export namespace RenderCore
{
    class PipelineManager final
    {
        static constexpr std::uint32_t g_NumDescriptorSetLayouts = 2U;

        VkRenderPass m_RenderPass;
        VkPipeline m_Pipeline;
        VkPipelineLayout m_PipelineLayout;
        VkPipelineCache m_PipelineCache;
        VkDescriptorPool m_DescriptorPool;
        std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
        std::vector<VkDescriptorSet> m_DescriptorSets;

    public:
        PipelineManager();

        PipelineManager(PipelineManager const&)            = delete;
        PipelineManager& operator=(PipelineManager const&) = delete;

        ~PipelineManager();

        static PipelineManager& Get();

        void CreateRenderPass();
        void CreateDefaultRenderPass();
        void CreateGraphicsPipeline();
        void CreateDescriptorSetLayout();
        void CreateDescriptorPool();
        void CreateDescriptorSets();

        void Shutdown();
        void DestroyResources();

        [[nodiscard]] VkRenderPass const& GetRenderPass() const;
        [[nodiscard]] VkPipeline const& GetPipeline() const;
        [[nodiscard]] VkPipelineLayout const& GetPipelineLayout() const;
        [[nodiscard]] VkPipelineCache const& GetPipelineCache() const;
        [[nodiscard]] std::vector<VkDescriptorSetLayout> const& GetDescriptorSetLayouts() const;
        [[nodiscard]] VkDescriptorPool const& GetDescriptorPool() const;
        [[nodiscard]] std::vector<VkDescriptorSet> const& GetDescriptorSets() const;
    };
}// namespace RenderCore