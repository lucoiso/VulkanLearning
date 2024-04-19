// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <vector>
#include <memory>
#include <vma/vk_mem_alloc.h>

export module RenderCore.Runtime.Pipeline;

import RenderCore.Types.Allocation;
import RenderCore.Types.Object;

export namespace RenderCore
{
    struct PipelineDescriptorData
    {
        DescriptorData SceneData {};
        DescriptorData ModelData {};
        DescriptorData TextureData {};

        [[nodiscard]] bool IsValid() const;
        void               DestroyResources(VmaAllocator const &);

        void SetDescriptorLayoutSize();

        void SetupSceneBuffer(BufferAllocation const &);

        void SetupModelsBuffer(std::vector<std::shared_ptr<Object>> const &);
    };

    void CreatePipeline();

    void CreatePipelineLibraries();

    void SetupPipelineLayouts();

    void ReleasePipelineResources();

    void ReleaseDynamicPipelineResources();

    [[nodiscard]] VkPipeline const &GetMainPipeline();

    [[nodiscard]] VkPipelineLayout const &GetPipelineLayout();

    [[nodiscard]] PipelineDescriptorData &GetPipelineDescriptorData();
} // namespace RenderCore
