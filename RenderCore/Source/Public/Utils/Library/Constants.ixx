// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Utils.Constants;

export namespace RenderCore
{
    constexpr std::array<char const *, 0U> g_RequiredInstanceLayers{};

    constexpr std::array<char const *, 0U> g_RequiredDeviceLayers{};

    constexpr std::array<char const *, 0U> g_RequiredInstanceExtensions{};

    constexpr std::array g_RequiredDeviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                    VK_EXT_MESH_SHADER_EXTENSION_NAME,
                                                    VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
                                                    VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
                                                    VK_EXT_GRAPHICS_PIPELINE_LIBRARY_EXTENSION_NAME,
                                                    VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME};

    constexpr std::array<char const *, 0U> g_OptionalInstanceLayers{};

    constexpr std::array<char const *, 0U> g_OptionalDeviceLayers{};

    constexpr std::array<char const *, 0U> g_OptionalInstanceExtensions{};

    constexpr std::array<char const *, 0U> g_OptionalDeviceExtensions{};

    constexpr VkPipelineCreateFlags g_PipelineFlags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT;

    constexpr std::array g_DynamicStates{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_LINE_WIDTH};

    constexpr std::array g_PreferredImageFormats{VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_A8B8G8R8_UNORM_PACK32};
    constexpr std::array g_PreferredDepthFormats{VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT};

    constexpr auto g_MapMemoryFlag = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    constexpr auto g_StagingMemoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;

    constexpr auto g_DescriptorMemoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    constexpr auto g_ModelMemoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    constexpr auto g_ModelBufferUsage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    constexpr auto g_TextureMemoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    constexpr VkSampleCountFlagBits g_MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    constexpr VkImageTiling         g_ImageTiling = VK_IMAGE_TILING_OPTIMAL;

    constexpr VkImageAspectFlags g_ImageAspect = VK_IMAGE_ASPECT_COLOR_BIT;
    constexpr VkImageAspectFlags g_DepthAspect = VK_IMAGE_ASPECT_DEPTH_BIT;

    constexpr VkImageLayout g_PresentLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    constexpr VkImageLayout g_UndefinedLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    constexpr VkImageLayout g_AttachmentLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;

    // NOTE: ImGui causing weird bug and forcing VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
    // Change to VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL after fixing it
    constexpr VkImageLayout g_ReadLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    constexpr std::uint8_t g_ImageCount = 3U;

    constexpr std::uint32_t g_Timeout = std::numeric_limits<std::uint32_t>::max();

    constexpr std::uint8_t g_MaxMeshletVertices = 64U;

    constexpr std::uint8_t g_MaxMeshletPrimitives = 126U;

    constexpr std::array g_ClearValues{VkClearValue{.color = {{0.F, 0.F, 0.F, 0.F}}}, VkClearValue{.depthStencil = {1.F, 0U}}};
} // namespace RenderCore
