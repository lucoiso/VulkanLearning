// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Runtime.Pipeline;

import RenderCore.Types.Allocation;
import RenderCore.Types.Object;

namespace RenderCore
{
    export struct RENDERCOREMODULE_API PipelineData
    {
        VkPipeline       MainPipeline { VK_NULL_HANDLE };
        VkPipeline       VertexInputPipeline { VK_NULL_HANDLE };
        VkPipeline       PreRasterizationPipeline { VK_NULL_HANDLE };
        VkPipeline       FragmentOutputPipeline { VK_NULL_HANDLE };
        VkPipeline       FragmentShaderPipeline { VK_NULL_HANDLE };
        VkPipelineLayout PipelineLayout { VK_NULL_HANDLE };
        VkPipelineCache  PipelineCache { VK_NULL_HANDLE };
        VkPipelineCache  PipelineLibraryCache { VK_NULL_HANDLE };

        [[nodiscard]] inline bool IsValid() const
        {
            return MainPipeline != VK_NULL_HANDLE || FragmentShaderPipeline != VK_NULL_HANDLE || VertexInputPipeline != VK_NULL_HANDLE ||
                   PreRasterizationPipeline != VK_NULL_HANDLE || FragmentOutputPipeline != VK_NULL_HANDLE || PipelineLayout != VK_NULL_HANDLE ||
                   PipelineCache != VK_NULL_HANDLE || PipelineLibraryCache != VK_NULL_HANDLE;
        }

        void DestroyResources(VkDevice const &, bool);

        void CreateMainCache(VkDevice const &);
        void CreateLibraryCache(VkDevice const &);
    };

    export struct RENDERCOREMODULE_API PipelineDescriptorData
    {
        DescriptorData SceneData {};
        DescriptorData ModelData {};
        DescriptorData TextureData {};

        [[nodiscard]] inline bool IsValid() const
        {
            return SceneData.IsValid() && ModelData.IsValid() && TextureData.IsValid();
        }

        void DestroyResources(VmaAllocator const &, bool);

        void SetDescriptorLayoutSize();
        void SetupSceneBuffer(BufferAllocation const &);
        void SetupModelsBuffer(std::vector<std::shared_ptr<Object>> const &);
    };

    export extern RENDERCOREMODULE_API PipelineData           g_PipelineData { VK_NULL_HANDLE };
    export extern RENDERCOREMODULE_API PipelineDescriptorData g_DescriptorData {};
}

export namespace RenderCore
{
    void CreatePipelineDynamicResources();
    void CreatePipelineLibraries();
    void SetupPipelineLayouts();
    void ReleasePipelineResources(bool);

    struct RENDERCOREMODULE_API PipelineLibraryCreationArguments
    {
        VkPipelineRasterizationStateCreateInfo         RasterizationState {};
        VkPipelineColorBlendAttachmentState            ColorBlendAttachment {};
        VkPipelineMultisampleStateCreateInfo           MultisampleState {};
        VkVertexInputBindingDescription                VertexBinding {};
        std::vector<VkVertexInputAttributeDescription> VertexAttributes {};
        std::vector<VkPipelineShaderStageCreateInfo>   ShaderStages {};
    };

    RENDERCOREMODULE_API void CreatePipelineLibraries(PipelineData &, PipelineLibraryCreationArguments const &, VkPipelineCreateFlags, bool);
    RENDERCOREMODULE_API void CreateMainPipeline(PipelineData &,
                                                 std::vector<VkPipelineShaderStageCreateInfo> const &,
                                                 VkPipelineCreateFlags,
                                                 VkPipelineDepthStencilStateCreateInfo const &,
                                                 VkPipelineMultisampleStateCreateInfo const &);

    RENDERCOREMODULE_API [[nodiscard]] inline VkPipeline const &GetMainPipeline()
    {
        return g_PipelineData.MainPipeline;
    }

    RENDERCOREMODULE_API [[nodiscard]] inline VkPipelineCache const &GetPipelineCache()
    {
        return g_PipelineData.PipelineCache;
    }

    RENDERCOREMODULE_API [[nodiscard]] inline VkPipelineLayout const &GetPipelineLayout()
    {
        return g_PipelineData.PipelineLayout;
    }

    RENDERCOREMODULE_API [[nodiscard]] inline PipelineDescriptorData &GetPipelineDescriptorData()
    {
        return g_DescriptorData;
    }
} // namespace RenderCore
