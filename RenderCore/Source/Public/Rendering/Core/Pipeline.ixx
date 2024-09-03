// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Runtime.Pipeline;

import RenderCore.Types.Allocation;
import RenderCore.Types.Object;

export namespace RenderCore
{
    struct PipelineData
    {
        VkPipeline       MainPipeline { VK_NULL_HANDLE };
        VkPipeline       VertexInputPipeline { VK_NULL_HANDLE };
        VkPipeline       PreRasterizationPipeline { VK_NULL_HANDLE };
        VkPipeline       FragmentOutputPipeline { VK_NULL_HANDLE };
        VkPipeline       FragmentShaderPipeline { VK_NULL_HANDLE };
        VkPipelineLayout PipelineLayout { VK_NULL_HANDLE };
        VkPipelineCache  PipelineCache { VK_NULL_HANDLE };
        VkPipelineCache  PipelineLibraryCache { VK_NULL_HANDLE };

        [[nodiscard]] bool IsValid() const;
        void               DestroyResources(VkDevice const &, bool);

        void CreateMainCache(VkDevice const &);
        void CreateLibraryCache(VkDevice const &);
    };

    struct PipelineDescriptorData
    {
        DescriptorData SceneData {};
        DescriptorData ModelData {};
        DescriptorData TextureData {};

        [[nodiscard]] bool IsValid() const;
        void               DestroyResources(VmaAllocator const &, bool);

        void SetDescriptorLayoutSize();
        void SetupSceneBuffer(BufferAllocation const &);
        void SetupModelsBuffer(std::vector<std::shared_ptr<Object>> const &);
    };

    void CreatePipelineDynamicResources();
    void CreatePipelineLibraries();
    void SetupPipelineLayouts();
    void ReleasePipelineResources(bool);

    [[nodiscard]] VkPipeline const &      GetMainPipeline();
    [[nodiscard]] VkPipelineCache const & GetPipelineCache();
    [[nodiscard]] VkPipelineLayout const &GetPipelineLayout();
    [[nodiscard]] PipelineDescriptorData &GetPipelineDescriptorData();

    struct PipelineLibraryCreationArguments
    {
        VkPipelineRasterizationStateCreateInfo         RasterizationState {};
        VkPipelineColorBlendAttachmentState            ColorBlendAttachment {};
        VkPipelineMultisampleStateCreateInfo           MultisampleState {};
        VkVertexInputBindingDescription                VertexBinding {};
        std::vector<VkVertexInputAttributeDescription> VertexAttributes {};
        std::vector<VkPipelineShaderStageCreateInfo>   ShaderStages {};
    };

    void CreatePipelineLibraries(PipelineData &, PipelineLibraryCreationArguments const &, VkPipelineCreateFlags, bool);
    void CreateMainPipeline(PipelineData &,
                            std::vector<VkPipelineShaderStageCreateInfo> const &,
                            VkPipelineCreateFlags,
                            VkPipelineDepthStencilStateCreateInfo const &,
                            VkPipelineMultisampleStateCreateInfo const &);
} // namespace RenderCore
