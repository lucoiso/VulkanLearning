// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <Volk/volk.h>

export module RenderCore.Runtime.Pipeline;

export namespace RenderCore
{
    void CreatePipeline();

    void CreateDescriptorSetLayout();

    void ReleasePipelineResources();

    [[nodiscard]] VkPipeline const &GetMainPipeline();

    [[nodiscard]] VkPipelineLayout const &GetPipelineLayout();
} // namespace RenderCore
