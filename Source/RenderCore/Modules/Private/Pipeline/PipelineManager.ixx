// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <volk.h>

export module RenderCore.Management.PipelineManagement;

export import <vector>;
export import <unordered_map>;

export import RenderCore.Types.MeshBufferData;

namespace RenderCore
{
    export class PipelineManager
    {
        VkRenderPass m_RenderPass {VK_NULL_HANDLE};
        VkPipeline m_Pipeline {VK_NULL_HANDLE};
        VkPipelineLayout m_PipelineLayout {VK_NULL_HANDLE};
        VkPipelineCache m_PipelineCache {VK_NULL_HANDLE};
        VkDescriptorPool m_DescriptorPool {VK_NULL_HANDLE};
        VkDescriptorSetLayout m_DescriptorSetLayout {VK_NULL_HANDLE};
        std::unordered_map<std::uint32_t, VkDescriptorSet> m_DescriptorSets {};

    public:
        void CreateRenderPass();
        void CreateGraphicsPipeline();
        void CreateDescriptorSetLayout();
        void CreateDescriptorPool(std::uint32_t);
        void CreateDescriptorSets(std::vector<MeshBufferData> const&);
        void ReleasePipelineResources();
        void ReleaseDynamicPipelineResources();

        [[nodiscard]] VkRenderPass const& GetRenderPass();
        [[nodiscard]] VkPipeline const& GetPipeline();
        [[nodiscard]] VkPipelineLayout const& GetPipelineLayout();
        [[nodiscard]] VkPipelineCache const& GetPipelineCache();
        [[nodiscard]] VkDescriptorSetLayout const& GetDescriptorSetLayout();
        [[nodiscard]] VkDescriptorPool const& GetDescriptorPool();
        [[nodiscard]] VkDescriptorSet GetDescriptorSet(std::uint32_t);
    };
}// namespace RenderCore