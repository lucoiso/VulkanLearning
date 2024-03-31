// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <Volk/volk.h>

export module RenderCore.Runtime.Pipeline;

export namespace RenderCore
{
    void CreatePipeline(VkFormat, VkFormat, VkExtent2D const &);

    void CreateDescriptorSetLayout();

    void ReleasePipelineResources();

    void ReleaseDynamicPipelineResources();

    [[nodiscard]] VkPipeline const &GetMainPipeline();

    [[nodiscard]] VkPipelineLayout const &GetPipelineLayout();

    [[nodiscard]] VkDescriptorSetLayout const &GetDescriptorSetLayout();
} // namespace RenderCore
