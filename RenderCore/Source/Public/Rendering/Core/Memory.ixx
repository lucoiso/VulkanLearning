// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Runtime.Memory;

import RenderCore.Runtime.Scene;
import RenderCore.Types.Allocation;
import RenderCore.Types.Object;
import RenderCore.Types.Texture;
import RenderCore.Utils.Constants;
import RenderCore.Utils.EnumHelpers;
import RenderCore.Utils.Helpers;

namespace RenderCore
{
    VmaPool g_StagingUniformBufferPool{VK_NULL_HANDLE};
    VmaPool g_StagingStorageBufferPool{VK_NULL_HANDLE};
            
    VmaPool g_DescriptorBufferPool{VK_NULL_HANDLE};
    VmaPool g_UniformPool{VK_NULL_HANDLE};
    VmaPool g_StoragePool{VK_NULL_HANDLE};
    VmaPool g_ImagePool{VK_NULL_HANDLE};

    VmaAllocator g_Allocator{VK_NULL_HANDLE};
                     
    BufferAllocation g_UniformAllocation{};
    BufferAllocation g_StorageAllocation{};

    std::atomic<std::uint64_t>                         g_ImageAllocationIDCounter{0U};
    std::unordered_map<std::uint32_t, ImageAllocation> g_AllocatedImages{};
    std::unordered_map<std::uint32_t, std::uint32_t>   g_ImageAllocationCounter{};
} // namespace RenderCore

export namespace RenderCore
{
    void CreateMemoryAllocator();
    void ReleaseMemoryResources();

    VmaAllocationInfo CreateBuffer(VkDeviceSize const &, VkBufferUsageFlags, strzilla::string_view, VkBuffer &, VmaAllocation &);
    void              CopyBuffer(VkCommandBuffer const &, VkBuffer const &, VkBuffer const &, VkDeviceSize const &);
    void              CreateUniformBuffers(BufferAllocation &, VkDeviceSize, strzilla::string_view);

    void CreateImage(VkFormat const &, VkExtent2D const &, VkImageTiling const &, VkImageUsageFlags, VmaMemoryUsage, strzilla::string_view, VkImage &, VmaAllocation &);
    void CreateImageView(VkImage const &, VkFormat const &, VkImageAspectFlags const &, VkImageView &);
    void CreateTextureImageView(ImageAllocation &, VkFormat);
    void CopyBufferToImage(VkCommandBuffer const &, VkBuffer const &, VkImage const &, VkExtent2D const &);

    [[nodiscard]] std::tuple<std::uint32_t, VkBuffer, VmaAllocation>
    AllocateTexture(VkCommandBuffer const &, unsigned char const *, std::uint32_t, std::uint32_t, VkFormat, VkDeviceSize);

    void AllocateModelsBuffers(std::vector<std::shared_ptr<Object>> const &);

    template <VkImageLayout OldLayout, VkImageLayout NewLayout, VkImageAspectFlags Aspect>
    RENDERCOREMODULE_API constexpr VkImageMemoryBarrier2 MountImageBarrier(VkImage const      &Image,
                                                      VkFormat const     &Format,
                                                      std::uint32_t const FromQueueIndex = VK_QUEUE_FAMILY_IGNORED,
                                                      std::uint32_t const ToQueueIndex   = VK_QUEUE_FAMILY_IGNORED)
    {
        VkImageMemoryBarrier2 ImageBarrier{.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                           .srcAccessMask       = 0U,
                                           .dstAccessMask       = 0U,
                                           .oldLayout           = OldLayout,
                                           .newLayout           = NewLayout,
                                           .srcQueueFamilyIndex = FromQueueIndex,
                                           .dstQueueFamilyIndex = ToQueueIndex,
                                           .image               = Image,
                                           .subresourceRange    = {.aspectMask     = Aspect,
                                                                   .baseMipLevel   = 0U,
                                                                   .levelCount     = VK_REMAINING_MIP_LEVELS,
                                                                   .baseArrayLayer = 0U,
                                                                   .layerCount     = VK_REMAINING_ARRAY_LAYERS}};

        if constexpr (HasFlag<VkImageAspectFlags>(Aspect, g_DepthAspect))
        {
            if (DepthHasStencil(Format))
            {
                ImageBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }

        if constexpr (OldLayout == g_UndefinedLayout && NewLayout == g_AttachmentLayout)
        {
            if constexpr (HasFlag<VkImageAspectFlags>(Aspect, g_DepthAspect))
            {
                ImageBarrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                ImageBarrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
                ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
            }
            else
            {
                ImageBarrier.srcAccessMask = VK_ACCESS_2_NONE;
                ImageBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
                ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            }
        }
        else if constexpr (OldLayout == g_UndefinedLayout && NewLayout == g_ReadLayout)
        {
            ImageBarrier.srcAccessMask = VK_ACCESS_2_NONE;
            ImageBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
            ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_NONE;
            ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        }
        else if constexpr (OldLayout == g_AttachmentLayout && NewLayout == g_ReadLayout)
        {
            ImageBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            ImageBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
            ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        }
        else if constexpr (OldLayout == g_AttachmentLayout && NewLayout == g_PresentLayout)
        {
            ImageBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            ImageBarrier.dstAccessMask = VK_ACCESS_2_NONE;
            ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        }
        else if constexpr (OldLayout == g_UndefinedLayout && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            ImageBarrier.srcAccessMask = VK_ACCESS_2_NONE;
            ImageBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_NONE;
            ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        }
        else if constexpr (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == g_ReadLayout)
        {
            ImageBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            ImageBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
            ImageBarrier.srcStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            ImageBarrier.dstStageMask  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        }

        return ImageBarrier;
    }

    template <VkImageLayout OldLayout, VkImageLayout NewLayout, VkImageAspectFlags Aspect>
    RENDERCOREMODULE_API constexpr void RequestImageLayoutTransition(VkCommandBuffer const &CommandBuffer,
                                                VkImage const         &Image,
                                                VkFormat const        &Format,
                                                std::uint32_t const    FromQueueIndex = VK_QUEUE_FAMILY_IGNORED,
                                                std::uint32_t const    ToQueueIndex   = VK_QUEUE_FAMILY_IGNORED)
    {
        VkImageMemoryBarrier2 ImageBarrier = MountImageBarrier<OldLayout, NewLayout, Aspect>(Image, Format, FromQueueIndex, ToQueueIndex);

        VkDependencyInfo const DependencyInfo{.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                              .dependencyFlags         = VK_DEPENDENCY_BY_REGION_BIT,
                                              .imageMemoryBarrierCount = 1U,
                                              .pImageMemoryBarriers    = &ImageBarrier};

        vkCmdPipelineBarrier2(CommandBuffer, &DependencyInfo);
    }

    RENDERCOREMODULE_API void SaveImageToFile(VkImage const &, strzilla::string_view, VkExtent2D const &);

    struct TextureDeleter
    {
        void operator()(Texture *Texture) const;
    };

    RENDERCOREMODULE_API void PrintMemoryAllocatorStats(bool);

    RENDERCOREMODULE_API [[nodiscard]] strzilla::string GetMemoryAllocatorStats(bool);

    RENDERCOREMODULE_API [[nodiscard]] inline VmaAllocator const &GetAllocator()
    {
        return g_Allocator;
    }

    RENDERCOREMODULE_API [[nodiscard]] inline BufferAllocation const& GetUniformAllocation()
    {
        return g_UniformAllocation;
    }

    RENDERCOREMODULE_API [[nodiscard]] inline BufferAllocation const& GetStorageAllocation()
    {
        return g_StorageAllocation;
    }

    RENDERCOREMODULE_API [[nodiscard]] inline VkDescriptorImageInfo GetAllocationImageDescriptor(std::uint32_t const Index)
    {
        return VkDescriptorImageInfo{.sampler = GetSampler(), .imageView = g_AllocatedImages.at(Index).View, .imageLayout = g_ReadLayout};
    }
} // namespace RenderCore
