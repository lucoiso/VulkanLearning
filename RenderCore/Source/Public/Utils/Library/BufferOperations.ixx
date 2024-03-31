// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <Volk/volk.h>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <vma/vk_mem_alloc.h>

export module RenderCore.Runtime.Buffer.Operations;

import RenderCore.Types.Allocation;
import RenderCore.Types.Vertex;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.EnumHelpers;

export namespace RenderCore
{
    VmaAllocationInfo CreateBuffer(VkDeviceSize const &, VkBufferUsageFlags, VkMemoryPropertyFlags, std::string_view, VkBuffer &, VmaAllocation &);

    void CopyBuffer(VkCommandBuffer const &, VkBuffer const &, VkBuffer const &, VkDeviceSize const &);

    std::pair<VkBuffer, VmaAllocation> CreateVertexBuffers(VkCommandBuffer &CommandBuffer, ObjectAllocationData &, std::vector<Vertex> const &);

    std::pair<VkBuffer, VmaAllocation> CreateIndexBuffers(VkCommandBuffer &CommandBuffer, ObjectAllocationData &, std::vector<std::uint32_t> const &);

    void CreateUniformBuffers(BufferAllocation &, VkDeviceSize, std::string_view);

    void CreateModelUniformBuffers(ObjectAllocationData &);

    void CreateImage(VkFormat const &,
                     VkExtent2D const &,
                     VkImageTiling const &,
                     VkImageUsageFlags,
                     VmaAllocationCreateFlags,
                     VmaMemoryUsage,
                     std::string_view,
                     VkImage &,
                     VmaAllocation &);

    void CreateTextureSampler(VkPhysicalDevice const &, VkSampler &);

    void CreateImageView(VkImage const &, VkFormat const &, VkImageAspectFlags const &, VkImageView &);

    void CreateSwapChainImageViews(std::vector<ImageAllocation> &, VkFormat);

    void CreateTextureImageView(ImageAllocation &, VkFormat);

    void CopyBufferToImage(VkCommandBuffer const &, VkBuffer const &, VkImage const &, VkExtent2D const &);

    std::pair<VkBuffer, VmaAllocation>
    AllocateTexture(VkCommandBuffer &, unsigned char const *, std::uint32_t, std::uint32_t, VkFormat, std::size_t, ImageAllocation &);

    template <VkImageLayout OldLayout, VkImageLayout NewLayout, VkImageAspectFlags Aspect>
    constexpr void MoveImageLayout(VkCommandBuffer    &CommandBuffer,
                                   VkImage const      &Image,
                                   VkFormat const     &Format,
                                   std::uint32_t const FromQueueIndex = VK_QUEUE_FAMILY_IGNORED,
                                   std::uint32_t const ToQueueIndex   = VK_QUEUE_FAMILY_IGNORED)
    {
        VkImageMemoryBarrier2 ImageBarrier {.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                            .srcAccessMask       = 0U,
                                            .dstAccessMask       = 0U,
                                            .oldLayout           = OldLayout,
                                            .newLayout           = NewLayout,
                                            .srcQueueFamilyIndex = FromQueueIndex,
                                            .dstQueueFamilyIndex = ToQueueIndex,
                                            .image               = Image,
                                            .subresourceRange
                                            = {.aspectMask = Aspect, .baseMipLevel = 0U, .levelCount = 1U, .baseArrayLayer = 0U, .layerCount = 1U}};

        if constexpr (HasFlag<VkImageAspectFlags>(Aspect, VK_IMAGE_ASPECT_DEPTH_BIT))
        {
            if (DepthHasStencil(Format))
            {
                ImageBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }

        if constexpr (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL)
        {
            if constexpr (HasFlag<VkImageAspectFlags>(Aspect, VK_IMAGE_ASPECT_DEPTH_BIT))
            {
                ImageBarrier.srcAccessMask = VK_ACCESS_2_NONE;
                ImageBarrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
            }
            else
            {
                ImageBarrier.srcAccessMask = VK_ACCESS_2_NONE;
                ImageBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_NONE;
                ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            }
        }
        else if constexpr (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL)
        {
            ImageBarrier.srcAccessMask = VK_ACCESS_2_NONE;
            ImageBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
            ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_NONE;
            ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        }
        else if constexpr (OldLayout == VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL)
        {
            ImageBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            ImageBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
            ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        }
        else if constexpr (OldLayout == VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        {
            ImageBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            ImageBarrier.dstAccessMask = VK_ACCESS_2_NONE;
            ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_NONE;
        }
        else if constexpr (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            ImageBarrier.srcAccessMask = VK_ACCESS_2_NONE;
            ImageBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_NONE;
            ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        }
        else if constexpr (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL)
        {
            ImageBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            ImageBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
            ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        }
        else
        {
            throw std::runtime_error("Unsupported layout transition!");
        }

        VkDependencyInfo const DependencyInfo {.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                               .imageMemoryBarrierCount = 1U,
                                               .pImageMemoryBarriers    = &ImageBarrier};

        vkCmdPipelineBarrier2(CommandBuffer, &DependencyInfo);
    }
} // namespace RenderCore
