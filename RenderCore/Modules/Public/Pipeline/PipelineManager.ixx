// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <unordered_map>
#include <vector>
#include <volk.h>

export module RenderCore.Management.PipelineManagement;

import RenderCore.Types.MeshBufferData;
import RenderCore.Types.AllocationTypes;
import RenderCore.Types.SurfaceProperties;
import RenderCore.Utils.Constants;

namespace RenderCore
{
    export class PipelineManager
    {
        VkRenderPass m_MainRenderPass {VK_NULL_HANDLE};
        VkPipeline m_MainPipeline {VK_NULL_HANDLE};

        VkRenderPass m_ViewportRenderPass {VK_NULL_HANDLE};
        VkPipeline m_ViewportPipeline {VK_NULL_HANDLE};

        VkPipelineLayout m_PipelineLayout {VK_NULL_HANDLE};
        VkPipelineCache m_PipelineCache {VK_NULL_HANDLE};
        VkDescriptorPool m_DescriptorPool {VK_NULL_HANDLE};
        VkDescriptorSetLayout m_DescriptorSetLayout {VK_NULL_HANDLE};
        std::unordered_map<std::uint32_t, VkDescriptorSet> m_DescriptorSets {};

    public:
        static void CreateRenderPass(SurfaceProperties const&, VkImageLayout, VkRenderPass&);
        void CreateRenderPasses(SurfaceProperties const&);

        static void CreatePipeline(VkRenderPass const&, VkPipelineLayout const&, VkExtent2D const&, VkPipeline&);
        void CreatePipelines(VkExtent2D const&);

        void CreateDescriptorSetLayout();
        void CreateDescriptorPool(std::uint32_t);
        void CreateDescriptorSets(std::vector<MeshBufferData> const&, VkSampler const&);

        void ReleasePipelineResources();
        void ReleaseDynamicPipelineResources();

        [[nodiscard]] VkRenderPass const& GetMainRenderPass() const;
        [[nodiscard]] VkRenderPass const& GetViewportRenderPass() const;

        [[nodiscard]] VkPipeline const& GetMainPipeline() const;
        [[nodiscard]] VkPipeline const& GetViewportPipeline() const;

        [[nodiscard]] VkPipelineLayout const& GetPipelineLayout() const;
        [[nodiscard]] VkDescriptorSetLayout const& GetDescriptorSetLayout() const;
        [[nodiscard]] VkDescriptorPool const& GetDescriptorPool() const;
        [[nodiscard]] VkDescriptorSet GetDescriptorSet(std::uint32_t) const;
    };
}// namespace RenderCore