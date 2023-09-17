// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace RenderCore
{
    struct VulkanBufferRecordParameters
    {
        const VkRenderPass RenderPass;
        const VkPipeline Pipeline;
        const VkExtent2D Extent;
        const std::vector<VkFramebuffer> FrameBuffers;
        const std::vector<VkBuffer> VertexBuffers;
        const std::vector<VkBuffer> IndexBuffers;
        const VkPipelineLayout PipelineLayout;
        const std::vector<VkDescriptorSet> DescriptorSets;
        const std::uint32_t IndexCount;
        const std::uint32_t ImageIndex;
        const std::vector<VkDeviceSize> Offsets;
    };
}