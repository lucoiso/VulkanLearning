// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <volk.h>

export module RenderCore.Management.PipelineManagement;

import <vector>;
import <unordered_map>;

import RenderCore.Types.MeshBufferData;
import RenderCore.Types.SurfaceProperties;

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
        bool m_BoundToImGui {false};

    public:
        void CreateRenderPass(SurfaceProperties const&);
        void CreateGraphicsPipeline();
        void CreateDescriptorSetLayout();
        void CreateDescriptorPool(std::uint32_t);
        void CreateDescriptorSets(std::vector<MeshBufferData> const&);
        void ReleasePipelineResources();
        void ReleaseDynamicPipelineResources();

        void SetIsBoundToImGui(bool);
        [[nodiscard]] bool GetIsBoundToImGui() const;

        [[nodiscard]] VkRenderPass const& GetRenderPass() const;
        [[nodiscard]] VkPipeline const& GetPipeline() const;
        [[nodiscard]] VkPipelineLayout const& GetPipelineLayout() const;
        [[nodiscard]] VkPipelineCache const& GetPipelineCache() const;
        [[nodiscard]] VkDescriptorSetLayout const& GetDescriptorSetLayout() const;
        [[nodiscard]] VkDescriptorPool const& GetDescriptorPool() const;
        [[nodiscard]] VkDescriptorSet GetDescriptorSet(std::uint32_t) const;
    };
}// namespace RenderCore