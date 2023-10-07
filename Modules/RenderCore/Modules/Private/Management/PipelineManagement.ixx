// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#pragma once

#include <volk.h>

export module RenderCore.Management.PipelineManagement;

import <vector>;
import <array>;

namespace RenderCore
{
    export void CreateRenderPass();
    export void CreateGraphicsPipeline();
    export void CreateDescriptorSetLayout();
    export void CreateDescriptorPool();
    export void CreateDescriptorSets();
    export void ReleasePipelineResources();

    export [[nodiscard]] VkRenderPass const& GetRenderPass();
    export [[nodiscard]] VkPipeline const& GetPipeline();
    export [[nodiscard]] VkPipelineLayout const& GetPipelineLayout();
    export [[nodiscard]] std::vector<VkDescriptorSet> const& GetDescriptorSets();
}// namespace RenderCore