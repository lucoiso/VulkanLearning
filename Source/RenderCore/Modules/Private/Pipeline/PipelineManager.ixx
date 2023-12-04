// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <volk.h>

export module RenderCore.Management.PipelineManagement;

export import <vector>;
export import <unordered_map>;

export import RenderCore.Types.MeshBufferData;
export import RenderCore.Types.DeviceProperties;

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
        void CreateRenderPass(VkDevice const&, DeviceProperties const&);
        void CreateGraphicsPipeline(VkDevice const&);
        void CreateDescriptorSetLayout(VkDevice const&);
        void CreateDescriptorPool(VkDevice const&, std::uint32_t);
        void CreateDescriptorSets(VkDevice const&, std::vector<MeshBufferData> const&);
        void ReleasePipelineResources(VkDevice const&);
        void ReleaseDynamicPipelineResources(VkDevice const&);

        void SetIsBoundToImGui(bool);
        [[nodiscard]] bool GetIsBoundToImGui() const;

        [[nodiscard]] VkRenderPass const& GetRenderPass();
        [[nodiscard]] VkPipeline const& GetPipeline();
        [[nodiscard]] VkPipelineLayout const& GetPipelineLayout();
        [[nodiscard]] VkPipelineCache const& GetPipelineCache();
        [[nodiscard]] VkDescriptorSetLayout const& GetDescriptorSetLayout();
        [[nodiscard]] VkDescriptorPool const& GetDescriptorPool();
        [[nodiscard]] VkDescriptorSet GetDescriptorSet(std::uint32_t);
    };
}// namespace RenderCore