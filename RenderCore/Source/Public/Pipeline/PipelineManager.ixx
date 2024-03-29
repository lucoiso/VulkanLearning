// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <Volk/volk.h>
#include <unordered_map>
#include <vector>

export module RenderCore.Management.PipelineManagement;

import RenderCore.Types.AllocationTypes;
import RenderCore.Types.SurfaceProperties;
import RenderCore.Utils.Constants;

namespace RenderCore
{
    export class PipelineManager
    {
        VkPipeline            m_Pipeline {VK_NULL_HANDLE};
        VkPipelineLayout      m_PipelineLayout {VK_NULL_HANDLE};
        VkPipelineCache       m_PipelineCache {VK_NULL_HANDLE};
        VkDescriptorSetLayout m_DescriptorSetLayout {VK_NULL_HANDLE};

    public:
        void CreatePipeline(VkFormat, VkFormat, VkExtent2D const &);

        void CreateDescriptorSetLayout();

        void ReleasePipelineResources();

        void ReleaseDynamicPipelineResources();

        [[nodiscard]] VkPipeline const &GetMainPipeline() const;

        [[nodiscard]] VkPipelineLayout const &GetPipelineLayout() const;

        [[nodiscard]] VkDescriptorSetLayout const &GetDescriptorSetLayout() const;
    };
} // namespace RenderCore
