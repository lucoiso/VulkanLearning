// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <array>
#include <cstdint>
#include <limits>
#include <vma/vk_mem_alloc.h>

export module RenderCore.Utils.Constants;

export namespace RenderCore
{
    constexpr std::array<char const *, 0U> g_RequiredInstanceLayers {};

    constexpr std::array<char const *, 0U> g_RequiredDeviceLayers {};

    constexpr std::array<char const *, 0U> g_RequiredInstanceExtensions {};

    constexpr std::array g_RequiredDeviceExtensions {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
            VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
            VK_EXT_MESH_SHADER_EXTENSION_NAME
    };

    constexpr std::array<char const *, 0U> g_OptionalInstanceLayers {};

    constexpr std::array<char const *, 0U> g_OptionalDeviceLayers {};

    constexpr std::array<char const *, 0U> g_OptionalInstanceExtensions {};

    constexpr std::array g_OptionalDeviceExtensions { VK_EXT_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_EXTENSION_NAME };

    constexpr std::array g_DynamicStates { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_LINE_WIDTH };

    constexpr std::uint64_t g_MinMemoryBlock             = 2U;
    constexpr std::uint64_t g_BufferMemoryAllocationSize = 2U * 1024U * 1024U; // 2MB
    constexpr std::uint64_t g_ImageMemoryAllocationSize  = g_BufferMemoryAllocationSize * 8U;

    constexpr auto g_ModelMemoryUsage   = VMA_MEMORY_USAGE_CPU_TO_GPU;
    constexpr auto g_ModelMemoryFlags   = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    constexpr auto g_TextureMemoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;

    constexpr VkSampleCountFlagBits g_MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    constexpr VkImageTiling         g_ImageTiling = VK_IMAGE_TILING_OPTIMAL;

    constexpr std::uint8_t g_MinImageCount = 3U;

    constexpr std::uint32_t g_Timeout = std::numeric_limits<std::uint32_t>::max();

    constexpr std::array g_ClearValues {
            VkClearValue { .color = { { 0.F, 0.F, 0.F, 1.F } } },
            VkClearValue { .color = { { 0.01F, 0.01F, 0.01F, 1.F } } },
            VkClearValue { .depthStencil = { 1.F, 0U } }
    };
} // namespace RenderCore
