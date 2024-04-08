// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <Volk/volk.h>
#include <array>
#include <cstdint>
#include <limits>
#include "RenderCoreModule.hpp"

export module RenderCore.Utils.Constants;

export namespace RenderCore
{
        constexpr bool g_EnableExperimentalFrustumCulling = false;

        constexpr std::array<char const *, 0U> g_RequiredInstanceLayers = {};

        constexpr std::array<char const *, 0U> g_RequiredDeviceLayers = {};

        constexpr std::array<char const *, 0U> g_RequiredInstanceExtensions = {};

        constexpr std::array g_RequiredDeviceExtensions = {
                        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                        VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
                        VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
                        VK_EXT_MESH_SHADER_EXTENSION_NAME
        };

        constexpr std::array<char const *, 0U> g_OptionalInstanceLayers = {};

        constexpr std::array<char const *, 0U> g_OptionalDeviceLayers = {};

        constexpr std::array<char const *, 0U> g_OptionalInstanceExtensions = {};

        constexpr std::array g_OptionalDeviceExtensions = { VK_EXT_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_EXTENSION_NAME };

        constexpr std::array g_DynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_LINE_WIDTH };

        constexpr std::uint64_t g_BufferMemoryAllocationSize = 65536U;

        constexpr std::uint64_t g_ImageBufferMemoryAllocationSize = 262144U;

        constexpr VkSampleCountFlagBits g_MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        constexpr std::uint8_t g_MinImageCount = 3U;

        constexpr std::uint32_t g_Timeout = std::numeric_limits<std::uint32_t>::max();

        RENDERCOREMODULE_API constexpr std::array g_ClearValues {
                        VkClearValue { .color = { { 0.F, 0.F, 0.F, 1.F } } },
                        VkClearValue { .color = { { 0.01F, 0.01F, 0.01F, 1.F } } },
                        VkClearValue { .depthStencil = { 1.F, 0U } }
        };
} // namespace RenderCore
