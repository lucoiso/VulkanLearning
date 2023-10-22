// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <volk.h>

export module RenderCore.Management.PipelineManagement;

export import <vector>;

namespace RenderCore
{
    export void CreateRenderPass();
    export void CreateGraphicsPipeline();
    export void CreateDescriptorSetLayout();
    export void CreateDescriptorPool();
    export void CreateDescriptorSets();
    export void ReleasePipelineResources();
    export void ReleaseDynamicPipelineResources();

    export [[nodiscard]] VkRenderPass const& GetRenderPass();
    export [[nodiscard]] VkPipeline const& GetPipeline();
    export [[nodiscard]] VkPipelineLayout const& GetPipelineLayout();
    export [[nodiscard]] VkPipelineCache const& GetPipelineCache();
    export [[nodiscard]] VkDescriptorSetLayout const& GetDescriptorSetLayout();
    export [[nodiscard]] VkDescriptorPool const& GetDescriptorPool();
    export [[nodiscard]] std::vector<VkDescriptorSet> const& GetDescriptorSets();
}// namespace RenderCore