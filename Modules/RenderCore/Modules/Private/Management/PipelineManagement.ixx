// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <volk.h>

export module RenderCore.Management.PipelineManagement;

import <vector>;
import <array>;

export namespace RenderCore
{
    void CreateRenderPass();
    void CreateGraphicsPipeline();
    void CreateDescriptorSetLayout();
    void CreateDescriptorPool();
    void CreateDescriptorSets();

    void ReleasePipelineResources();

    [[nodiscard]] VkRenderPass const& GetRenderPass();
    [[nodiscard]] VkPipeline const& GetPipeline();
    [[nodiscard]] VkPipelineLayout const& GetPipelineLayout();
    [[nodiscard]] std::vector<VkDescriptorSet> const& GetDescriptorSets();
}// namespace RenderCore