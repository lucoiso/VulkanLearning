// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

// Adapted from: https://github.com/ocornut/imgui/blob/docking/backends/imgui_impl_vulkan.h

module;

module RenderCore.Integrations.ImGuiVulkanBackend;

import RenderCore.Types.Allocation;
import RenderCore.Types.SurfaceProperties;
import RenderCore.Utils.Constants;
import RenderCore.Utils.Helpers;
import RenderCore.Renderer;
import RenderCore.Runtime.Memory;
import RenderCore.Runtime.Device;
import RenderCore.Runtime.Instance;
import RenderCore.Runtime.Pipeline;
import RenderCore.Runtime.SwapChain;
import RenderCore.Runtime.Command;
import RenderCore.Runtime.Scene;
import ThreadPool;

using namespace RenderCore;

namespace ShaderData
{
    // backends/vulkan/glsl_shader.vert, compiled with:
    // # glslangValidator -V -x -o glsl_shader.vert.u32 glsl_shader.vert
    /*
    #version 450 core
    layout(location = 0) in vec2 aPos;
    layout(location = 1) in vec2 aUV;
    layout(location = 2) in vec4 aColor;
    layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;

    out gl_PerVertex { vec4 gl_Position; };
    layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;

    void main()
    {
        Out.Color = aColor;
        Out.UV = aUV;
        gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
    }
    */
    constexpr std::array<std::uint32_t, 324U> g_ImGuiVertexShaderBin {
            0x07230203,
            0x00010000,
            0x00080001,
            0x0000002e,
            0x00000000,
            0x00020011,
            0x00000001,
            0x0006000b,
            0x00000001,
            0x4c534c47,
            0x6474732e,
            0x3035342e,
            0x00000000,
            0x0003000e,
            0x00000000,
            0x00000001,
            0x000a000f,
            0x00000000,
            0x00000004,
            0x6e69616d,
            0x00000000,
            0x0000000b,
            0x0000000f,
            0x00000015,
            0x0000001b,
            0x0000001c,
            0x00030003,
            0x00000002,
            0x000001c2,
            0x00040005,
            0x00000004,
            0x6e69616d,
            0x00000000,
            0x00030005,
            0x00000009,
            0x00000000,
            0x00050006,
            0x00000009,
            0x00000000,
            0x6f6c6f43,
            0x00000072,
            0x00040006,
            0x00000009,
            0x00000001,
            0x00005655,
            0x00030005,
            0x0000000b,
            0x0074754f,
            0x00040005,
            0x0000000f,
            0x6c6f4361,
            0x0000726f,
            0x00030005,
            0x00000015,
            0x00565561,
            0x00060005,
            0x00000019,
            0x505f6c67,
            0x65567265,
            0x78657472,
            0x00000000,
            0x00060006,
            0x00000019,
            0x00000000,
            0x505f6c67,
            0x7469736f,
            0x006e6f69,
            0x00030005,
            0x0000001b,
            0x00000000,
            0x00040005,
            0x0000001c,
            0x736f5061,
            0x00000000,
            0x00060005,
            0x0000001e,
            0x73755075,
            0x6e6f4368,
            0x6e617473,
            0x00000074,
            0x00050006,
            0x0000001e,
            0x00000000,
            0x61635375,
            0x0000656c,
            0x00060006,
            0x0000001e,
            0x00000001,
            0x61725475,
            0x616c736e,
            0x00006574,
            0x00030005,
            0x00000020,
            0x00006370,
            0x00040047,
            0x0000000b,
            0x0000001e,
            0x00000000,
            0x00040047,
            0x0000000f,
            0x0000001e,
            0x00000002,
            0x00040047,
            0x00000015,
            0x0000001e,
            0x00000001,
            0x00050048,
            0x00000019,
            0x00000000,
            0x0000000b,
            0x00000000,
            0x00030047,
            0x00000019,
            0x00000002,
            0x00040047,
            0x0000001c,
            0x0000001e,
            0x00000000,
            0x00050048,
            0x0000001e,
            0x00000000,
            0x00000023,
            0x00000000,
            0x00050048,
            0x0000001e,
            0x00000001,
            0x00000023,
            0x00000008,
            0x00030047,
            0x0000001e,
            0x00000002,
            0x00020013,
            0x00000002,
            0x00030021,
            0x00000003,
            0x00000002,
            0x00030016,
            0x00000006,
            0x00000020,
            0x00040017,
            0x00000007,
            0x00000006,
            0x00000004,
            0x00040017,
            0x00000008,
            0x00000006,
            0x00000002,
            0x0004001e,
            0x00000009,
            0x00000007,
            0x00000008,
            0x00040020,
            0x0000000a,
            0x00000003,
            0x00000009,
            0x0004003b,
            0x0000000a,
            0x0000000b,
            0x00000003,
            0x00040015,
            0x0000000c,
            0x00000020,
            0x00000001,
            0x0004002b,
            0x0000000c,
            0x0000000d,
            0x00000000,
            0x00040020,
            0x0000000e,
            0x00000001,
            0x00000007,
            0x0004003b,
            0x0000000e,
            0x0000000f,
            0x00000001,
            0x00040020,
            0x00000011,
            0x00000003,
            0x00000007,
            0x0004002b,
            0x0000000c,
            0x00000013,
            0x00000001,
            0x00040020,
            0x00000014,
            0x00000001,
            0x00000008,
            0x0004003b,
            0x00000014,
            0x00000015,
            0x00000001,
            0x00040020,
            0x00000017,
            0x00000003,
            0x00000008,
            0x0003001e,
            0x00000019,
            0x00000007,
            0x00040020,
            0x0000001a,
            0x00000003,
            0x00000019,
            0x0004003b,
            0x0000001a,
            0x0000001b,
            0x00000003,
            0x0004003b,
            0x00000014,
            0x0000001c,
            0x00000001,
            0x0004001e,
            0x0000001e,
            0x00000008,
            0x00000008,
            0x00040020,
            0x0000001f,
            0x00000009,
            0x0000001e,
            0x0004003b,
            0x0000001f,
            0x00000020,
            0x00000009,
            0x00040020,
            0x00000021,
            0x00000009,
            0x00000008,
            0x0004002b,
            0x00000006,
            0x00000028,
            0x00000000,
            0x0004002b,
            0x00000006,
            0x00000029,
            0x3f800000,
            0x00050036,
            0x00000002,
            0x00000004,
            0x00000000,
            0x00000003,
            0x000200f8,
            0x00000005,
            0x0004003d,
            0x00000007,
            0x00000010,
            0x0000000f,
            0x00050041,
            0x00000011,
            0x00000012,
            0x0000000b,
            0x0000000d,
            0x0003003e,
            0x00000012,
            0x00000010,
            0x0004003d,
            0x00000008,
            0x00000016,
            0x00000015,
            0x00050041,
            0x00000017,
            0x00000018,
            0x0000000b,
            0x00000013,
            0x0003003e,
            0x00000018,
            0x00000016,
            0x0004003d,
            0x00000008,
            0x0000001d,
            0x0000001c,
            0x00050041,
            0x00000021,
            0x00000022,
            0x00000020,
            0x0000000d,
            0x0004003d,
            0x00000008,
            0x00000023,
            0x00000022,
            0x00050085,
            0x00000008,
            0x00000024,
            0x0000001d,
            0x00000023,
            0x00050041,
            0x00000021,
            0x00000025,
            0x00000020,
            0x00000013,
            0x0004003d,
            0x00000008,
            0x00000026,
            0x00000025,
            0x00050081,
            0x00000008,
            0x00000027,
            0x00000024,
            0x00000026,
            0x00050051,
            0x00000006,
            0x0000002a,
            0x00000027,
            0x00000000,
            0x00050051,
            0x00000006,
            0x0000002b,
            0x00000027,
            0x00000001,
            0x00070050,
            0x00000007,
            0x0000002c,
            0x0000002a,
            0x0000002b,
            0x00000028,
            0x00000029,
            0x00050041,
            0x00000011,
            0x0000002d,
            0x0000001b,
            0x0000000d,
            0x0003003e,
            0x0000002d,
            0x0000002c,
            0x000100fd,
            0x00010038
    };

    // backends/vulkan/glsl_shader.frag, compiled with:
    // # glslangValidator -V -x -o glsl_shader.frag.u32 glsl_shader.frag
    /*
    #version 450 core
    layout(location = 0) out vec4 fColor;
    layout(set=0, binding=0) uniform Sampler2D sTexture;
    layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
    void main()
    {
        fColor = In.Color * texture(sTexture, In.UV.st);
    }
    */
    constexpr std::array<std::uint32_t, 193U> g_ImGuiFragmentShaderBin {
            0x07230203,
            0x00010000,
            0x00080001,
            0x0000001e,
            0x00000000,
            0x00020011,
            0x00000001,
            0x0006000b,
            0x00000001,
            0x4c534c47,
            0x6474732e,
            0x3035342e,
            0x00000000,
            0x0003000e,
            0x00000000,
            0x00000001,
            0x0007000f,
            0x00000004,
            0x00000004,
            0x6e69616d,
            0x00000000,
            0x00000009,
            0x0000000d,
            0x00030010,
            0x00000004,
            0x00000007,
            0x00030003,
            0x00000002,
            0x000001c2,
            0x00040005,
            0x00000004,
            0x6e69616d,
            0x00000000,
            0x00040005,
            0x00000009,
            0x6c6f4366,
            0x0000726f,
            0x00030005,
            0x0000000b,
            0x00000000,
            0x00050006,
            0x0000000b,
            0x00000000,
            0x6f6c6f43,
            0x00000072,
            0x00040006,
            0x0000000b,
            0x00000001,
            0x00005655,
            0x00030005,
            0x0000000d,
            0x00006e49,
            0x00050005,
            0x00000016,
            0x78655473,
            0x65727574,
            0x00000000,
            0x00040047,
            0x00000009,
            0x0000001e,
            0x00000000,
            0x00040047,
            0x0000000d,
            0x0000001e,
            0x00000000,
            0x00040047,
            0x00000016,
            0x00000022,
            0x00000000,
            0x00040047,
            0x00000016,
            0x00000021,
            0x00000000,
            0x00020013,
            0x00000002,
            0x00030021,
            0x00000003,
            0x00000002,
            0x00030016,
            0x00000006,
            0x00000020,
            0x00040017,
            0x00000007,
            0x00000006,
            0x00000004,
            0x00040020,
            0x00000008,
            0x00000003,
            0x00000007,
            0x0004003b,
            0x00000008,
            0x00000009,
            0x00000003,
            0x00040017,
            0x0000000a,
            0x00000006,
            0x00000002,
            0x0004001e,
            0x0000000b,
            0x00000007,
            0x0000000a,
            0x00040020,
            0x0000000c,
            0x00000001,
            0x0000000b,
            0x0004003b,
            0x0000000c,
            0x0000000d,
            0x00000001,
            0x00040015,
            0x0000000e,
            0x00000020,
            0x00000001,
            0x0004002b,
            0x0000000e,
            0x0000000f,
            0x00000000,
            0x00040020,
            0x00000010,
            0x00000001,
            0x00000007,
            0x00090019,
            0x00000013,
            0x00000006,
            0x00000001,
            0x00000000,
            0x00000000,
            0x00000000,
            0x00000001,
            0x00000000,
            0x0003001b,
            0x00000014,
            0x00000013,
            0x00040020,
            0x00000015,
            0x00000000,
            0x00000014,
            0x0004003b,
            0x00000015,
            0x00000016,
            0x00000000,
            0x0004002b,
            0x0000000e,
            0x00000018,
            0x00000001,
            0x00040020,
            0x00000019,
            0x00000001,
            0x0000000a,
            0x00050036,
            0x00000002,
            0x00000004,
            0x00000000,
            0x00000003,
            0x000200f8,
            0x00000005,
            0x00050041,
            0x00000010,
            0x00000011,
            0x0000000d,
            0x0000000f,
            0x0004003d,
            0x00000007,
            0x00000012,
            0x00000011,
            0x0004003d,
            0x00000014,
            0x00000017,
            0x00000016,
            0x00050041,
            0x00000019,
            0x0000001a,
            0x0000000d,
            0x00000018,
            0x0004003d,
            0x0000000a,
            0x0000001b,
            0x0000001a,
            0x00050057,
            0x00000007,
            0x0000001c,
            0x00000017,
            0x0000001b,
            0x00050085,
            0x00000007,
            0x0000001d,
            0x00000012,
            0x0000001c,
            0x0003003e,
            0x00000009,
            0x0000001d,
            0x000100fd,
            0x00010038
    };

    constexpr VkShaderModuleCreateInfo g_VertexShaderInfo {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = static_cast<std::uint32_t>(std::size(g_ImGuiVertexShaderBin) * sizeof(std::uint32_t)),
            .pCode = std::data(g_ImGuiVertexShaderBin)
    };

    constexpr VkShaderModuleCreateInfo g_FragmentShaderInfo {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = static_cast<std::uint32_t>(std::size(g_ImGuiFragmentShaderBin) * sizeof(std::uint32_t)),
            .pCode = std::data(g_ImGuiFragmentShaderBin)
    };

    constexpr VkPipelineShaderStageCreateInfo g_VertexShaderStage {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = &g_VertexShaderInfo,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .pName = "main"
    };

    constexpr VkPipelineShaderStageCreateInfo g_FragmentShaderStage {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = &g_FragmentShaderInfo,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pName = "main"
    };
} // namespace ShaderData

bool ImGuiVulkanSecondaryCommands::IsValid() const
{
    for (RenderCore::ThreadResources const &ThreadResourceIt : ThreadResources)
    {
        if (ThreadResourceIt.CommandPool == VK_NULL_HANDLE || ThreadResourceIt.CommandPool == VK_NULL_HANDLE)
        {
            return false;
        }
    }

    return true;
}

void ImGuiVulkanSecondaryCommands::Initialize(VkDevice const &LogicalDevice, std::uint8_t const QueueFamilyIndex)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    ThreadResources.resize(std::thread::hardware_concurrency());

    for (RenderCore::ThreadResources &ThreadResourceIt : ThreadResources)
    {
        ThreadResourceIt.Allocate(LogicalDevice, QueueFamilyIndex);
    }
}

void ImGuiVulkanSecondaryCommands::Free(VkDevice const &LogicalDevice)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    Wait();

    for (RenderCore::ThreadResources &ThreadResourceIt : ThreadResources)
    {
        ThreadResourceIt.Free(LogicalDevice);
    }
}

void ImGuiVulkanSecondaryCommands::Destroy(VkDevice const &LogicalDevice)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    Wait();

    for (RenderCore::ThreadResources &ThreadResourceIt : ThreadResources)
    {
        ThreadResourceIt.Destroy(LogicalDevice);
    }
}

void ImGuiVulkanSecondaryCommands::Reset(VkDevice const &LogicalDevice)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    Wait();

    for (RenderCore::ThreadResources &ThreadResourceIt : ThreadResources)
    {
        ThreadResourceIt.Reset(LogicalDevice);
    }
}

void ImGuiVulkanSecondaryCommands::Wait()
{
    EASY_FUNCTION(profiler::colors::Blue50);

    ThreadPool::Pool const &CommandThreadPool = RenderCore::GetThreadPool();
    CommandThreadPool.Wait();
}

struct ImGuiVulkanFrame
{
    VkCommandPool   CommandPool { VK_NULL_HANDLE };
    VkCommandBuffer CommandBuffer { VK_NULL_HANDLE };
    VkFence         Fence { VK_NULL_HANDLE };
    bool            PendingWait { false };
    ImageAllocation Backbuffer {};
};

struct ImGuiVulkanFrameSemaphores
{
    VkSemaphore ImageAcquiredSemaphore { VK_NULL_HANDLE };
    VkSemaphore RenderCompleteSemaphore { VK_NULL_HANDLE };
};

struct ImGuiVulkanWindow
{
    std::uint32_t                                          Width {};
    std::uint32_t                                          Height {};
    VkSwapchainKHR                                         Swapchain { VK_NULL_HANDLE };
    VkSurfaceKHR                                           Surface { VK_NULL_HANDLE };
    std::uint32_t                                          FrameIndex {};
    std::uint32_t                                          SemaphoreIndex {};
    std::array<ImGuiVulkanFrame, g_ImageCount>             Frames {};
    std::array<ImGuiVulkanFrameSemaphores, g_ImageCount>   FrameSemaphores {};
    std::array<ImGuiVulkanSecondaryCommands, g_ImageCount> SecondaryCommands {};
};

struct ImGuiVulkanWindowRenderBuffers
{
    std::uint32_t                              Index { 0U };
    std::array<BufferAllocation, g_ImageCount> Buffers {};
};

struct ImGuiVulkanViewportData
{
    bool                           WindowOwned { false };
    ImGuiVulkanWindow              Window {};
    ImGuiVulkanWindowRenderBuffers RenderBuffers {};
};

struct ImGuiVulkanData
{
    ImGuiVulkanInitInfo            VulkanInitInfo {};
    VkDescriptorSetLayout          DescriptorSetLayout {};
    PipelineData                   PipelineData {};
    VkSampler                      FontSampler {};
    ImageAllocation                FontImage {};
    VkDescriptorSet                FontDescriptorSet {};
    ImGuiVulkanWindowRenderBuffers MainWindowRenderBuffers {};
};

ImGuiVulkanData *ImGuiVulkanGetBackendData()
{
    return ImGui::GetCurrentContext() ? static_cast<ImGuiVulkanData *>(ImGui::GetIO().BackendRendererUserData) : nullptr;
}

void ImGuiVulkanSetupRenderState(ImDrawData const *const &DrawData,
                                 VkPipeline const &       Pipeline,
                                 VkCommandBuffer const &  CommandBuffer,
                                 BufferAllocation const & RenderBuffers)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    if (DrawData->TotalVtxCount <= 0)
    {
        return;
    }

    ImGuiVulkanData const *Backend = ImGuiVulkanGetBackendData();

    vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);

    constexpr VkDeviceSize VertexOffset    = 0U;
    VkDeviceSize const     VertexBufferEnd = sizeof(ImDrawVert) * DrawData->TotalVtxCount;

    vkCmdBindVertexBuffers(CommandBuffer, 0U, 1U, &RenderBuffers.Buffer, &VertexOffset);
    vkCmdBindIndexBuffer(CommandBuffer, RenderBuffers.Buffer, VertexBufferEnd, VK_INDEX_TYPE_UINT16);

    float const ScaleX = 2.F / DrawData->DisplaySize.x;
    float const ScaleY = 2.F / DrawData->DisplaySize.y;

    constexpr std::uint32_t NumData = 4U;
    std::array const        ConstantData { ScaleX, ScaleY, -1.F - DrawData->DisplayPos.x * ScaleX, -1.F - DrawData->DisplayPos.y * ScaleY };

    vkCmdPushConstants(CommandBuffer,
                       Backend->PipelineData.PipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT,
                       0U,
                       NumData * sizeof(float),
                       std::data(ConstantData));
}

void ImGuiVulkanCreatePipeline()
{
    EASY_FUNCTION(profiler::colors::Blue50);

    constexpr VkVertexInputBindingDescription VertexBindingDescription {
            .binding = 0U,
            .stride = sizeof(ImDrawVert),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    constexpr VkPipelineRasterizationStateCreateInfo RasterInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .lineWidth = 1.F
    };

    constexpr VkPipelineColorBlendAttachmentState ColorAttachment {
            .blendEnable = VK_TRUE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    constexpr VkPipelineMultisampleStateCreateInfo MsInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = g_MSAASamples
    };

    constexpr VkPipelineDepthStencilStateCreateInfo DepthInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_FALSE,
            .depthWriteEnable = VK_FALSE,
            .depthCompareOp = VK_COMPARE_OP_NEVER,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE
    };

    PipelineLibraryCreationArguments const Arguments {
            .RasterizationState = RasterInfo,
            .ColorBlendAttachment = ColorAttachment,
            .MultisampleState = MsInfo,
            .VertexBinding = VertexBindingDescription,
            .VertexAttributes = {
                    VkVertexInputAttributeDescription
                    {
                            .location = 0U,
                            .binding = VertexBindingDescription.binding,
                            .format = VK_FORMAT_R32G32_SFLOAT,
                            .offset = offsetof(ImDrawVert, pos)
                    },
                    VkVertexInputAttributeDescription {
                            .location = 1U,
                            .binding = VertexBindingDescription.binding,
                            .format = VK_FORMAT_R32G32_SFLOAT,
                            .offset = offsetof(ImDrawVert, uv)
                    },
                    VkVertexInputAttributeDescription {
                            .location = 2U,
                            .binding = VertexBindingDescription.binding,
                            .format = VK_FORMAT_R8G8B8A8_UNORM,
                            .offset = offsetof(ImDrawVert, col)
                    }
            },
            .ShaderStages = { ShaderData::g_VertexShaderStage }
    };

    ImGuiVulkanData *Backend = ImGuiVulkanGetBackendData();

    CreatePipelineLibraries(Backend->PipelineData, Arguments, 0U);
    CreateMainPipeline(Backend->PipelineData, { ShaderData::g_FragmentShaderStage }, 0U, DepthInfo, MsInfo);
}

void ImGuiVulkanCreateWindow(ImGuiViewport *Viewport)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    VkInstance const &      Instance       = GetInstance();
    VkPhysicalDevice const &PhysicalDevice = GetPhysicalDevice();
    VkDevice const &        LogicalDevice  = GetLogicalDevice();

    auto const &[QueueFamilyIndex, Queue] = GetGraphicsQueue();

    ImGuiVulkanData const *Backend      = ImGuiVulkanGetBackendData();
    auto *                 ViewportData = IM_NEW(ImGuiVulkanViewportData)();

    ImGuiVulkanWindow &WindowData = ViewportData->Window;

    for (std::uint8_t ImageIt = 0U; ImageIt < g_ImageCount; ++ImageIt)
    {
        WindowData.SecondaryCommands.at(ImageIt).Initialize(LogicalDevice, RenderCore::GetGraphicsQueue().first);
    }

    Viewport->RendererUserData = ViewportData;

    ImGuiVulkanInitInfo const &VulkanInfo = Backend->VulkanInitInfo;

    ImGuiPlatformIO const &PlatformIO = ImGui::GetPlatformIO();
    CheckVulkanResult(static_cast<VkResult>(PlatformIO.Platform_CreateVkSurface(Viewport,
                                                                                reinterpret_cast<ImU64>(Instance),
                                                                                nullptr,
                                                                                reinterpret_cast<ImU64 *>(&WindowData.Surface))));

    VkBool32 SupportResult = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, QueueFamilyIndex, WindowData.Surface, &SupportResult);

    ImVector<VkFormat> SurfaceFormats;

    for (std::int32_t FormatIt = 0; FormatIt < VulkanInfo.PipelineRenderingCreateInfo.colorAttachmentCount; ++FormatIt)
    {
        SurfaceFormats.push_back(VulkanInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats[FormatIt]);
    }

    for (VkFormat const FormatIt : g_PreferredImageFormats)
    {
        SurfaceFormats.push_back(FormatIt);
    }

    ImGuiVulkanCreateOrResizeWindow(WindowData, static_cast<std::int32_t>(Viewport->Size.x), static_cast<std::int32_t>(Viewport->Size.y));
    ViewportData->WindowOwned = true;
}

void ImGuiVulkanDestroyViewport(ImGuiViewport *Viewport)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    if (auto *ViewportData = static_cast<ImGuiVulkanViewportData *>(Viewport->RendererUserData))
    {
        if (ViewportData->WindowOwned)
        {
            ImGuiVulkanDestroyWindow(ViewportData->Window);
        }

        ImGuiVulkanDestroyWindowRenderBuffers(ViewportData->RenderBuffers);
        IM_DELETE(ViewportData);
    }

    Viewport->RendererUserData = nullptr;
}

void ImGuiVulkanSetWindowSize(ImGuiViewport *Viewport, ImVec2 const Size)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    auto *ViewportData = static_cast<ImGuiVulkanViewportData *>(Viewport->RendererUserData);
    if (ViewportData == nullptr)
    {
        return;
    }

    ImGuiVulkanCreateOrResizeWindow(ViewportData->Window, static_cast<std::int32_t>(Size.x), static_cast<std::int32_t>(Size.y));
}

void ImGuiVulkanRenderWindow(ImGuiViewport *Viewport, void *)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    VkDevice const &LogicalDevice             = GetLogicalDevice();
    auto const &    [QueueFamilyIndex, Queue] = GetGraphicsQueue();

    auto *             ViewportData = static_cast<ImGuiVulkanViewportData *>(Viewport->RendererUserData);
    ImGuiVulkanWindow &WindowData   = ViewportData->Window;

    for (ImGuiVulkanSecondaryCommands &CommandIt : WindowData.SecondaryCommands)
    {
        if (!CommandIt.IsValid())
        {
            CommandIt.Initialize(LogicalDevice, RenderCore::GetGraphicsQueue().first);
        }
    }

    auto &      [CommandPool, CommandBuffer, Fence, PendingWait, BackBuffer] = WindowData.Frames.at(WindowData.FrameIndex);
    auto const &[ImageAcquiredSemaphore, RenderCompleteSemaphore]            = WindowData.FrameSemaphores.at(WindowData.SemaphoreIndex);

    if constexpr (!g_ForceDefaultSync)
    {
        if (PendingWait)
        {
            CheckVulkanResult(vkWaitForFences(LogicalDevice, 1U, &Fence, VK_FALSE, g_Timeout));
            CheckVulkanResult(vkResetFences(LogicalDevice, 1U, &Fence));

            WindowData.SecondaryCommands.at(Renderer::GetImageIndex()).Reset(LogicalDevice);

            CheckVulkanResult(vkResetCommandPool(LogicalDevice, CommandPool, 0U));
            PendingWait = false;
        }
    }

    if (vkAcquireNextImageKHR(LogicalDevice, WindowData.Swapchain, g_Timeout, ImageAcquiredSemaphore, VK_NULL_HANDLE, &WindowData.FrameIndex) !=
        VK_SUCCESS)
    {
        return;
    }

    constexpr VkCommandBufferBeginInfo CommandBufferBeginInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    CheckVulkanResult(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo));

    RenderCore::RequestImageLayoutTransition<g_UndefinedLayout, g_AttachmentLayout,
                                             g_ImageAspect>(CommandBuffer, BackBuffer.Image, BackBuffer.Format);

    VkRenderingAttachmentInfo const AttachmentInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .imageView = BackBuffer.View,
            .imageLayout = g_AttachmentLayout,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = g_ClearValues.at(0U)
    };

    ImageAllocation const &DepthAllocation = RenderCore::GetDepthImage();

    VkRenderingAttachmentInfo const DepthAttachmentInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = DepthAllocation.View,
            .imageLayout = g_AttachmentLayout,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = g_ClearValues.at(1U)
    };

    VkRenderingInfo const RenderingInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
            .flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT,
            .renderArea = { { 0, 0 }, { static_cast<std::uint32_t>(Viewport->Size.x), static_cast<std::uint32_t>(Viewport->Size.y) } },
            .layerCount = 1U,
            .viewMask = 0U,
            .colorAttachmentCount = 1U,
            .pColorAttachments = &AttachmentInfo,
            .pDepthAttachment = &DepthAttachmentInfo,
            .pStencilAttachment = &DepthAttachmentInfo
    };

    vkCmdBeginRendering(CommandBuffer, &RenderingInfo);
    ImGuiVulkanRenderDrawData(Viewport->DrawData, CommandBuffer);
    vkCmdEndRendering(CommandBuffer);

    RenderCore::RequestImageLayoutTransition<g_AttachmentLayout, g_PresentLayout, g_ImageAspect>(CommandBuffer, BackBuffer.Image, BackBuffer.Format);

    CheckVulkanResult(vkEndCommandBuffer(CommandBuffer));

    VkSemaphoreSubmitInfo const WaitSemaphoreInfo {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = ImageAcquiredSemaphore,
            .value = 1U,
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSemaphoreSubmitInfo const SignalSemaphoreInfo {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = RenderCompleteSemaphore,
            .value = 1U,
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkCommandBufferSubmitInfo const PrimarySubmission { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = CommandBuffer };

    VkSubmitInfo2 const SubmitInfo {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .waitSemaphoreInfoCount = 1U,
            .pWaitSemaphoreInfos = &WaitSemaphoreInfo,
            .commandBufferInfoCount = 1U,
            .pCommandBufferInfos = &PrimarySubmission,
            .signalSemaphoreInfoCount = 1U,
            .pSignalSemaphoreInfos = &SignalSemaphoreInfo
    };

    CheckVulkanResult(vkQueueSubmit2(Queue, 1U, &SubmitInfo, Fence));
    PendingWait = true;

    if constexpr (g_ForceDefaultSync)
    {
        CheckVulkanResult(vkWaitForFences(LogicalDevice, 1U, &Fence, VK_FALSE, g_Timeout));
        CheckVulkanResult(vkResetFences(LogicalDevice, 1U, &Fence));

        WindowData.SecondaryCommands.at(Renderer::GetImageIndex()).Reset(LogicalDevice);

        CheckVulkanResult(vkResetCommandPool(LogicalDevice, CommandPool, 0U));
    }
}

void ImGuiVulkanSwapBuffers(ImGuiViewport *Viewport, void *)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    auto const &[QueueFamilyIndex, Queue] = GetGraphicsQueue();

    auto *             ViewportData = static_cast<ImGuiVulkanViewportData *>(Viewport->RendererUserData);
    ImGuiVulkanWindow &WindowData   = ViewportData->Window;

    std::uint32_t const               FrameIndex      = WindowData.FrameIndex;
    ImGuiVulkanFrameSemaphores const &FrameSemaphores = WindowData.FrameSemaphores.at(WindowData.SemaphoreIndex);

    VkPresentInfoKHR const PresentInfo {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1U,
            .pWaitSemaphores = &FrameSemaphores.RenderCompleteSemaphore,
            .swapchainCount = 1U,
            .pSwapchains = &WindowData.Swapchain,
            .pImageIndices = &FrameIndex
    };

    if (VkResult const Result = vkQueuePresentKHR(Queue, &PresentInfo);
        Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR)
    {
        ImGuiVulkanCreateOrResizeWindow(ViewportData->Window,
                                        static_cast<std::int32_t>(Viewport->Size.x),
                                        static_cast<std::int32_t>(Viewport->Size.y));
    }

    WindowData.FrameIndex     = (WindowData.FrameIndex + 1U) % g_ImageCount;
    WindowData.SemaphoreIndex = (WindowData.SemaphoreIndex + 1U) % static_cast<std::uint32_t>(std::size(WindowData.FrameSemaphores));
}

bool RenderCore::ImGuiVulkanCreateDeviceObjects()
{
    EASY_FUNCTION(profiler::colors::Blue50);

    ImGuiVulkanData *Backend       = ImGuiVulkanGetBackendData();
    VkDevice const & LogicalDevice = GetLogicalDevice();

    if (!Backend->FontSampler)
    {
        constexpr VkSamplerCreateInfo SamplerInfo {
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .magFilter = VK_FILTER_LINEAR,
                .minFilter = VK_FILTER_LINEAR,
                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .maxAnisotropy = 1.F,
                .minLod = -1000,
                .maxLod = 1000
        };
        CheckVulkanResult(vkCreateSampler(LogicalDevice, &SamplerInfo, nullptr, &Backend->FontSampler));
    }

    if (!Backend->DescriptorSetLayout)
    {
        VkDescriptorSetLayoutBinding const Binding {
                .binding = 0U,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1U,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        };

        VkDescriptorSetLayoutCreateInfo const Info {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = 1U,
                .pBindings = &Binding
        };

        CheckVulkanResult(vkCreateDescriptorSetLayout(LogicalDevice, &Info, nullptr, &Backend->DescriptorSetLayout));
    }

    if (!Backend->PipelineData.PipelineLayout)
    {
        constexpr VkPushConstantRange PushConstantRange { .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = 0U, .size = sizeof(float) * 4U };

        VkPipelineLayoutCreateInfo const LayoutInfo {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = 1U,
                .pSetLayouts = &Backend->DescriptorSetLayout,
                .pushConstantRangeCount = 1U,
                .pPushConstantRanges = &PushConstantRange
        };

        CheckVulkanResult(vkCreatePipelineLayout(LogicalDevice, &LayoutInfo, nullptr, &Backend->PipelineData.PipelineLayout));
    }

    ImGuiVulkanCreatePipeline();
    return true;
}

void RenderCore::ImGuiVulkanDestroyDeviceObjects()
{
    EASY_FUNCTION(profiler::colors::Blue50);

    ImGuiVulkanData *Backend       = ImGuiVulkanGetBackendData();
    VkDevice const & LogicalDevice = GetLogicalDevice();

    ImGuiVulkanDestroyAllViewportsRenderBuffers();
    ImGuiVulkanDestroyFontsTexture();

    if (Backend->FontSampler)
    {
        vkDestroySampler(LogicalDevice, Backend->FontSampler, nullptr);
        Backend->FontSampler = VK_NULL_HANDLE;
    }

    if (Backend->DescriptorSetLayout)
    {
        vkDestroyDescriptorSetLayout(LogicalDevice, Backend->DescriptorSetLayout, nullptr);
        Backend->DescriptorSetLayout = VK_NULL_HANDLE;
    }

    Backend->PipelineData.DestroyResources(LogicalDevice, true);
}

void RenderCore::ImGuiVulkanDestroyFrameRenderBuffers(BufferAllocation &FrameBuffers)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    FrameBuffers.DestroyResources(GetAllocator());
}

void RenderCore::ImGuiVulkanDestroyWindowRenderBuffers(ImGuiVulkanWindowRenderBuffers &Buffers)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    for (BufferAllocation &Buffer : Buffers.Buffers)
    {
        ImGuiVulkanDestroyFrameRenderBuffers(Buffer);
    }

    Buffers.Index = 0U;
}

void RenderCore::ImGuiVulkanCreateWindowCommandBuffers(ImGuiVulkanWindow &WindowData)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    VkDevice const &LogicalDevice             = GetLogicalDevice();
    auto const &    [QueueFamilyIndex, Queue] = GetGraphicsQueue();

    for (std::uint32_t Iterator = 0U; Iterator < g_ImageCount; ++Iterator)
    {
        ImGuiVulkanFrame &FrameData = WindowData.Frames.at(Iterator);

        VkCommandPoolCreateInfo const CommandPoolInfo {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                .queueFamilyIndex = QueueFamilyIndex
        };

        CheckVulkanResult(vkCreateCommandPool(LogicalDevice, &CommandPoolInfo, nullptr, &FrameData.CommandPool));

        VkCommandBufferAllocateInfo const CommandBufferInfo {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = FrameData.CommandPool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1U
        };

        CheckVulkanResult(vkAllocateCommandBuffers(LogicalDevice, &CommandBufferInfo, &FrameData.CommandBuffer));

        constexpr VkFenceCreateInfo FenceInfo { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };

        CheckVulkanResult(vkCreateFence(LogicalDevice, &FenceInfo, nullptr, &FrameData.Fence));
        CheckVulkanResult(vkResetFences(LogicalDevice, 1U, &FrameData.Fence));
    }

    for (std::uint32_t Iterator = 0U; Iterator < static_cast<std::uint32_t>(std::size(WindowData.FrameSemaphores)); Iterator++)
    {
        ImGuiVulkanFrameSemaphores &FrameSemaphore = WindowData.FrameSemaphores.at(Iterator);

        constexpr VkSemaphoreCreateInfo SemaphoreInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

        CheckVulkanResult(vkCreateSemaphore(LogicalDevice, &SemaphoreInfo, nullptr, &FrameSemaphore.ImageAcquiredSemaphore));
        CheckVulkanResult(vkCreateSemaphore(LogicalDevice, &SemaphoreInfo, nullptr, &FrameSemaphore.RenderCompleteSemaphore));
    }
}

void RenderCore::ImGuiVulkanCreateWindowSwapChain(ImGuiVulkanWindow &WindowData, std::int32_t const WindowWidth, std::int32_t const WindowHeight)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    if (WindowWidth <= 0 || WindowHeight <= 0)
    {
        return;
    }

    VkPhysicalDevice const &PhysicalDevice = GetPhysicalDevice();
    VkDevice const &        LogicalDevice  = GetLogicalDevice();

    VkSwapchainKHR const OldSwapchain = WindowData.Swapchain;
    WindowData.Swapchain              = VK_NULL_HANDLE;

    WindowData.FrameIndex = 0U;

    for (std::uint32_t Iterator = 0U; Iterator < g_ImageCount; Iterator++)
    {
        ImGuiVulkanDestroyFrame(WindowData.Frames.at(Iterator), WindowData.SecondaryCommands.at(Iterator));
    }

    for (std::uint32_t Iterator = 0U; Iterator < static_cast<std::uint32_t>(std::size(WindowData.FrameSemaphores)); Iterator++)
    {
        ImGuiVulkanDestroyFrameSemaphores(WindowData.FrameSemaphores.at(Iterator));
    }

    auto const &[Format, DepthFormat, Mode, Extent] = GetCachedSurfaceProperties();

    VkSwapchainCreateInfoKHR SwapchainCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = WindowData.Surface,
            .minImageCount = g_ImageCount,
            .imageFormat = Format.format,
            .imageColorSpace = Format.colorSpace,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = Mode,
            .clipped = VK_TRUE,
            .oldSwapchain = OldSwapchain
    };

    VkSurfaceCapabilitiesKHR SurfaceCapabilities;
    CheckVulkanResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, WindowData.Surface, &SurfaceCapabilities));

    if (SwapchainCreateInfo.minImageCount < SurfaceCapabilities.minImageCount)
    {
        SwapchainCreateInfo.minImageCount = SurfaceCapabilities.minImageCount;
    }
    else if (SurfaceCapabilities.maxImageCount != 0U && SwapchainCreateInfo.minImageCount > SurfaceCapabilities.maxImageCount)
    {
        SwapchainCreateInfo.minImageCount = SurfaceCapabilities.maxImageCount;
    }

    if (SurfaceCapabilities.currentExtent.width == 0xffffffff)
    {
        SwapchainCreateInfo.imageExtent.width  = WindowWidth;
        SwapchainCreateInfo.imageExtent.height = WindowHeight;
        WindowData.Width                       = WindowWidth;
        WindowData.Height                      = WindowHeight;
    }
    else
    {
        SwapchainCreateInfo.imageExtent.width  = SurfaceCapabilities.currentExtent.width;
        SwapchainCreateInfo.imageExtent.height = SurfaceCapabilities.currentExtent.height;
        WindowData.Width                       = SurfaceCapabilities.currentExtent.width;
        WindowData.Height                      = SurfaceCapabilities.currentExtent.height;
    }

    CheckVulkanResult(vkCreateSwapchainKHR(LogicalDevice, &SwapchainCreateInfo, nullptr, &WindowData.Swapchain));

    std::uint32_t Count = 0U;
    CheckVulkanResult(vkGetSwapchainImagesKHR(LogicalDevice, WindowData.Swapchain, &Count, nullptr));

    std::vector<VkImage> BackBuffers(Count, VK_NULL_HANDLE);
    CheckVulkanResult(vkGetSwapchainImagesKHR(LogicalDevice, WindowData.Swapchain, &Count, std::data(BackBuffers)));

    for (std::uint32_t Iterator = 0U; Iterator < g_ImageCount; Iterator++)
    {
        WindowData.Frames.at(Iterator).Backbuffer.Image  = BackBuffers.at(Iterator);
        WindowData.Frames.at(Iterator).Backbuffer.Extent = SwapchainCreateInfo.imageExtent;
        WindowData.Frames.at(Iterator).Backbuffer.Format = SwapchainCreateInfo.imageFormat;
    }

    if (OldSwapchain)
    {
        vkDestroySwapchainKHR(LogicalDevice, OldSwapchain, nullptr);
    }

    for (std::uint32_t Iterator = 0U; Iterator < g_ImageCount; Iterator++)
    {
        ImGuiVulkanFrame &FrameData = WindowData.Frames.at(Iterator);
        CreateImageView(FrameData.Backbuffer.Image, FrameData.Backbuffer.Format, g_ImageAspect, FrameData.Backbuffer.View);
    }
}

void RenderCore::ImGuiVulkanDestroyFrame(ImGuiVulkanFrame &FrameData, ImGuiVulkanSecondaryCommands &Commands)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    VkDevice const &LogicalDevice = GetLogicalDevice();

    Commands.Destroy(LogicalDevice);

    if constexpr (!g_ForceDefaultSync)
    {
        if (FrameData.PendingWait)
        {
            if (FrameData.CommandPool != VK_NULL_HANDLE)
            {
                CheckVulkanResult(vkResetCommandPool(LogicalDevice, FrameData.CommandPool, 0U));
            }

            if (FrameData.Fence != VK_NULL_HANDLE)
            {
                CheckVulkanResult(vkWaitForFences(LogicalDevice, 1U, &FrameData.Fence, VK_FALSE, g_Timeout));
                CheckVulkanResult(vkResetFences(LogicalDevice, 1U, &FrameData.Fence));
            }

            FrameData.PendingWait = false;
        }
    }

    if (FrameData.CommandPool != VK_NULL_HANDLE)
    {
        if (FrameData.CommandBuffer != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers(LogicalDevice, FrameData.CommandPool, 1U, &FrameData.CommandBuffer);
            FrameData.CommandBuffer = VK_NULL_HANDLE;
        }

        vkDestroyCommandPool(LogicalDevice, FrameData.CommandPool, nullptr);
        FrameData.CommandPool = VK_NULL_HANDLE;
    }

    if (FrameData.Fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(LogicalDevice, FrameData.Fence, nullptr);
        FrameData.Fence = VK_NULL_HANDLE;
    }

    VmaAllocator const &Allocator = GetAllocator();
    FrameData.Backbuffer.DestroyResources(Allocator);
}

void RenderCore::ImGuiVulkanDestroyFrameSemaphores(ImGuiVulkanFrameSemaphores &FrameSemaphore)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    auto const &[QueueFamilyIndex, Queue] = GetGraphicsQueue();
    CheckVulkanResult(vkQueueWaitIdle(Queue));

    VkDevice const &LogicalDevice = GetLogicalDevice();

    vkDestroySemaphore(LogicalDevice, FrameSemaphore.ImageAcquiredSemaphore, nullptr);
    vkDestroySemaphore(LogicalDevice, FrameSemaphore.RenderCompleteSemaphore, nullptr);

    FrameSemaphore.ImageAcquiredSemaphore  = VK_NULL_HANDLE;
    FrameSemaphore.RenderCompleteSemaphore = VK_NULL_HANDLE;
}

void RenderCore::ImGuiVulkanDestroyAllViewportsRenderBuffers()
{
    EASY_FUNCTION(profiler::colors::Blue50);

    CheckVulkanResult(vkDeviceWaitIdle(GetLogicalDevice()));

    ImGuiPlatformIO &PlatformIO = ImGui::GetPlatformIO();

    for (std::int32_t ViewportIt = 0; ViewportIt < PlatformIO.Viewports.Size; ++ViewportIt)
    {
        if (auto *ViewportData = static_cast<ImGuiVulkanViewportData *>(PlatformIO.Viewports[ViewportIt]->RendererUserData))
        {
            ImGuiVulkanDestroyWindowRenderBuffers(ViewportData->RenderBuffers);
        }
    }
}

void RenderCore::ImGuiVulkanInitPlatformInterface()
{
    EASY_FUNCTION(profiler::colors::Blue50);

    ImGuiPlatformIO &PlatformIO       = ImGui::GetPlatformIO();
    PlatformIO.Renderer_CreateWindow  = ImGuiVulkanCreateWindow;
    PlatformIO.Renderer_DestroyWindow = ImGuiVulkanDestroyViewport;
    PlatformIO.Renderer_SetWindowSize = ImGuiVulkanSetWindowSize;
    PlatformIO.Renderer_RenderWindow  = ImGuiVulkanRenderWindow;
    PlatformIO.Renderer_SwapBuffers   = ImGuiVulkanSwapBuffers;
}

void RenderCore::ImGuiVulkanShutdownPlatformInterface()
{
    EASY_FUNCTION(profiler::colors::Blue50);

    ImGui::DestroyPlatformWindows();
}

static void DrawRenderingData(VkCommandBuffer const  CommandBuffer,
                              ImGuiVulkanData const *Backend,
                              ImDrawData *const &    DrawData,
                              ImDrawCmd const &      DrawCmd,
                              std::uint32_t const    GlobalIdxOffset,
                              std::uint32_t const    GlobalVtxOffset)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    {
        VkViewport const Viewport {
                .x = 0.F,
                .y = 0.F,
                .width = DrawData->DisplaySize.x,
                .height = DrawData->DisplaySize.y,
                .minDepth = 0.F,
                .maxDepth = 1.F
        };
        vkCmdSetViewport(CommandBuffer, 0U, 1U, &Viewport);
    }

    ImVec2 const &DisplayPosition = DrawData->DisplayPos;
    ImVec2 const &BufferScale     = DrawData->FramebufferScale;

    ImVec2 MinClip((DrawCmd.ClipRect.x - DisplayPosition.x) * BufferScale.x, (DrawCmd.ClipRect.y - DisplayPosition.y) * BufferScale.y);
    ImVec2 MaxClip((DrawCmd.ClipRect.z - DisplayPosition.x) * BufferScale.x, (DrawCmd.ClipRect.w - DisplayPosition.y) * BufferScale.y);

    if (MinClip.x < 0.F)
    {
        MinClip.x = 0.F;
    }
    if (MinClip.y < 0.F)
    {
        MinClip.y = 0.F;
    }

    if (MaxClip.x > DrawData->DisplaySize.x)
    {
        MaxClip.x = DrawData->DisplaySize.x;
    }
    if (MaxClip.y > DrawData->DisplaySize.y)
    {
        MaxClip.y = DrawData->DisplaySize.y;
    }

    if (MaxClip.x <= MinClip.x || MaxClip.y <= MinClip.y)
    {
        return;
    }

    {
        VkRect2D const Scissor {
                .offset = { static_cast<std::int32_t>(MinClip.x), static_cast<std::int32_t>(MinClip.y) },
                .extent = { static_cast<std::uint32_t>(MaxClip.x - MinClip.x), static_cast<std::uint32_t>(MaxClip.y - MinClip.y) }
        };

        vkCmdSetScissor(CommandBuffer, 0U, 1U, &Scissor);
    }

    auto const DescriptorSet = static_cast<VkDescriptorSet>(DrawCmd.TextureId);

    vkCmdBindDescriptorSets(CommandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            Backend->PipelineData.PipelineLayout,
                            0U,
                            1U,
                            &DescriptorSet,
                            0U,
                            nullptr);

    vkCmdDrawIndexed(CommandBuffer,
                     DrawCmd.ElemCount,
                     1U,
                     DrawCmd.IdxOffset + GlobalIdxOffset,
                     static_cast<int32_t>(DrawCmd.VtxOffset + GlobalVtxOffset),
                     0U);
}

void RenderCore::ImGuiVulkanRenderDrawData(ImDrawData *const &DrawData, VkCommandBuffer const CommandBuffer)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    std::uint8_t const ImageIndex = RenderCore::Renderer::GetFrameIndex();
    if (DrawData->DisplaySize.x <= 0U || DrawData->DisplaySize.y <= 0U)
    {
        return;
    }

    ImGuiVulkanData const *Backend = ImGuiVulkanGetBackendData();

    auto *ViewportRenderData = static_cast<ImGuiVulkanViewportData *>(DrawData->OwnerViewport->RendererUserData);
    auto &[Index, Buffers]   = ViewportRenderData->RenderBuffers;

    Index                           = (Index + 1U) % static_cast<std::uint32_t>(std::size(Buffers));
    BufferAllocation &RenderBuffers = Buffers.at(Index);

    VmaAllocator const &Allocator = GetAllocator();

    if (DrawData->TotalVtxCount <= 0U)
    {
        if (RenderBuffers.IsValid())
        {
            RenderBuffers.DestroyResources(Allocator);
        }

        return;
    }

    VkDeviceSize const VertexSize = DrawData->TotalVtxCount * sizeof(ImDrawVert);
    VkDeviceSize const IndexSize  = DrawData->TotalIdxCount * sizeof(ImDrawIdx);
    VkDeviceSize const TotalSize  = VertexSize + IndexSize;

    if (VkDeviceSize const BufferSize = RenderBuffers.GetAllocationSize(Allocator);
        BufferSize > 0U && BufferSize < TotalSize)
    {
        RenderBuffers.DestroyResources(Allocator);
    }

    if (!RenderBuffers.IsValid())
    {
        constexpr VkBufferUsageFlags BufferUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        constexpr auto               BufferIdentifier = "IMGUI_RENDER";
        RenderBuffers.Size = TotalSize;
        [[maybe_unused]] auto const _ = CreateBuffer(TotalSize, BufferUsage, BufferIdentifier, RenderBuffers.Buffer, RenderBuffers.Allocation);
    }

    vmaMapMemory(Allocator, RenderBuffers.Allocation, &RenderBuffers.MappedData);
    {
        auto *VertexBuffer = static_cast<char *>(RenderBuffers.MappedData);
        auto *IndexBuffer  = static_cast<char *>(RenderBuffers.MappedData) + VertexSize;

        std::for_each(std::execution::unseq,
                      std::cbegin(DrawData->CmdLists),
                      std::cend(DrawData->CmdLists),
                      [&](ImDrawList *const &CmdList)
                      {
                          memcpy(VertexBuffer, CmdList->VtxBuffer.Data, CmdList->VtxBuffer.Size * sizeof(ImDrawVert));
                          VertexBuffer += CmdList->VtxBuffer.Size * sizeof(ImDrawVert);

                          memcpy(IndexBuffer, CmdList->IdxBuffer.Data, CmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
                          IndexBuffer += CmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
                      });

        CheckVulkanResult(vmaFlushAllocation(Allocator, RenderBuffers.Allocation, 0U, TotalSize));
    }
    vmaUnmapMemory(Allocator, RenderBuffers.Allocation);

    VkCommandBufferInheritanceRenderingInfo const InheritanceRenderingInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO,
            .flags = VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT,
            .colorAttachmentCount = Backend->VulkanInitInfo.PipelineRenderingCreateInfo.colorAttachmentCount,
            .pColorAttachmentFormats = Backend->VulkanInitInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats,
            .depthAttachmentFormat = Backend->VulkanInitInfo.PipelineRenderingCreateInfo.depthAttachmentFormat,
            .stencilAttachmentFormat = Backend->VulkanInitInfo.PipelineRenderingCreateInfo.depthAttachmentFormat,
            .rasterizationSamples = g_MSAASamples,
    };

    VkCommandBufferInheritanceInfo const InheritanceInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
            .pNext = &InheritanceRenderingInfo
    };

    VkCommandBufferBeginInfo const SecondaryBeginInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
            .pInheritanceInfo = &InheritanceInfo,
    };

    std::atomic<std::uint8_t> ThreadIndex { 0U };

    ThreadPool::Pool const &CommandThreadPool = RenderCore::GetThreadPool();
    std::vector             WorkerThreads(std::thread::hardware_concurrency(), false);

    ImGuiVulkanSecondaryCommands &SecondaryCommands = ViewportRenderData->Window.SecondaryCommands.at(ImageIndex);

    std::uint32_t GlobalIdxOffset = 0u;
    std::uint32_t GlobalVtxOffset = 0u;

    std::for_each(std::execution::unseq,
                  std::cbegin(DrawData->CmdLists),
                  std::cend(DrawData->CmdLists),
                  [&](ImDrawList *const &CmdList)
                  {
                      std::for_each(std::execution::unseq,
                                    std::cbegin(CmdList->CmdBuffer),
                                    std::cend(CmdList->CmdBuffer),
                                    [&](ImDrawCmd const &Buffer)
                                    {
                                        if (Buffer.ElemCount <= 0U)
                                        {
                                            return;
                                        }

                                        std::uint8_t MyIndex = ThreadIndex.fetch_add(1U) % std::size(WorkerThreads);

                                        CommandThreadPool.AddTask([&SecondaryBeginInfo, &SecondaryCommands, MyIndex, DrawData, Backend, RenderBuffers,
                                                                      Buffer, GlobalIdxOffset, GlobalVtxOffset]
                                                                  {
                                                                      ThreadResources const &Resource = SecondaryCommands.ThreadResources.at(MyIndex);
                                                                      VkCommandBuffer const &CommandBuffer = Resource.CommandBuffer;

                                                                      CheckVulkanResult(vkBeginCommandBuffer(CommandBuffer, &SecondaryBeginInfo));
                                                                      {
                                                                          ImGuiVulkanSetupRenderState(DrawData,
                                                                              Backend->PipelineData.MainPipeline,
                                                                              CommandBuffer,
                                                                              RenderBuffers);

                                                                          DrawRenderingData(CommandBuffer,
                                                                                            Backend,
                                                                                            DrawData,
                                                                                            Buffer,
                                                                                            GlobalIdxOffset,
                                                                                            GlobalVtxOffset);
                                                                      }
                                                                      CheckVulkanResult(vkEndCommandBuffer(CommandBuffer));
                                                                  },
                                                                  MyIndex);

                                        WorkerThreads.at(MyIndex) = true;
                                    });

                      GlobalIdxOffset += CmdList->IdxBuffer.Size;
                      GlobalVtxOffset += CmdList->VtxBuffer.Size;
                  });

    VkRect2D const Scissor {
            .offset = { 0, 0 },
            .extent = { static_cast<std::uint32_t>(DrawData->DisplaySize.x), static_cast<std::uint32_t>(DrawData->DisplaySize.y) }
    };
    vkCmdSetScissor(CommandBuffer, 0U, 1U, &Scissor);

    std::vector<VkCommandBuffer> SecondaryBuffers;
    SecondaryBuffers.reserve(std::size(WorkerThreads));

    for (std::uint8_t WorkerIt = 0U; WorkerIt < std::size(WorkerThreads); ++WorkerIt)
    {
        if (WorkerThreads.at(WorkerIt))
        {
            SecondaryBuffers.emplace_back(SecondaryCommands.ThreadResources.at(WorkerIt).CommandBuffer);
        }
        else
        {
            break;
        }
    }

    CommandThreadPool.Wait();

    if (!std::empty(SecondaryBuffers))
    {
        vkCmdExecuteCommands(CommandBuffer, static_cast<std::uint32_t>(std::size(SecondaryBuffers)), std::data(SecondaryBuffers));
    }
}

bool RenderCore::ImGuiVulkanCreateFontsTexture()
{
    EASY_FUNCTION(profiler::colors::Blue50);

    ImGuiIO const &  ImGuiIO = ImGui::GetIO();
    ImGuiVulkanData *Backend = ImGuiVulkanGetBackendData();

    constexpr VkFormat  ImageFormat               = VK_FORMAT_R8G8B8A8_UNORM;
    VmaAllocator const &Allocator                 = GetAllocator();
    auto const &        [QueueFamilyIndex, Queue] = GetGraphicsQueue();

    if (Backend->FontImage.IsValid() || Backend->FontDescriptorSet)
    {
        ImGuiVulkanDestroyFontsTexture();
    }

    unsigned char *ImageData;
    std::int32_t   ImageWidth;
    std::int32_t   ImageHeight;
    ImGuiIO.Fonts->GetTexDataAsRGBA32(&ImageData, &ImageWidth, &ImageHeight);
    std::size_t const BufferSize = ImageWidth * ImageHeight * 4U * sizeof(char);

    VkCommandPool                CommandPool { VK_NULL_HANDLE };
    std::vector<VkCommandBuffer> CommandBuffers { VK_NULL_HANDLE };

    InitializeSingleCommandQueue(CommandPool, CommandBuffers, QueueFamilyIndex);

    std::pair<VkBuffer, VmaAllocation> StagingAllocation;
    VmaAllocationInfo                  StagingInfo = CreateBuffer(BufferSize,
                                                                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                                  "STAGING_TEXTURE",
                                                                  StagingAllocation.first,
                                                                  StagingAllocation.second);

    CheckVulkanResult(vmaMapMemory(Allocator, StagingAllocation.second, &StagingInfo.pMappedData));
    std::memcpy(StagingInfo.pMappedData, ImageData, BufferSize);

    ImageAllocation NewAllocation {
            .Extent = { .width = static_cast<std::uint32_t>(ImageWidth), .height = static_cast<std::uint32_t>(ImageHeight) },
            .Format = ImageFormat
    };

    CreateImage(ImageFormat,
                NewAllocation.Extent,
                g_ImageTiling,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                g_TextureMemoryUsage,
                "TEXTURE",
                NewAllocation.Image,
                NewAllocation.Allocation);

    RequestImageLayoutTransition<g_UndefinedLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, g_ImageAspect>(CommandBuffers.back(),
        NewAllocation.Image,
        NewAllocation.Format);

    CopyBufferToImage(CommandBuffers.back(), StagingAllocation.first, NewAllocation.Image, NewAllocation.Extent);

    RequestImageLayoutTransition<VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, g_ReadLayout, g_ImageAspect>(CommandBuffers.back(),
                                                                                                        NewAllocation.Image,
                                                                                                        NewAllocation.Format);

    CreateImageView(NewAllocation.Image, NewAllocation.Format, g_ImageAspect, NewAllocation.View);
    vmaUnmapMemory(Allocator, StagingAllocation.second);

    FinishSingleCommandQueue(Queue, CommandPool, CommandBuffers);

    vmaDestroyBuffer(Allocator, StagingAllocation.first, StagingAllocation.second);

    Backend->FontImage = std::move(NewAllocation);

    Backend->FontDescriptorSet = ImGuiVulkanAddTexture(Backend->FontSampler, Backend->FontImage.View, g_ReadLayout);
    ImGuiIO.Fonts->SetTexID(Backend->FontDescriptorSet);

    return true;
}

void RenderCore::ImGuiVulkanDestroyFontsTexture()
{
    EASY_FUNCTION(profiler::colors::Blue50);

    ImGuiIO const &  ImGuiIO = ImGui::GetIO();
    ImGuiVulkanData *Backend = ImGuiVulkanGetBackendData();

    if (Backend->FontDescriptorSet)
    {
        ImGuiVulkanRemoveTexture(Backend->FontDescriptorSet);
        Backend->FontDescriptorSet = VK_NULL_HANDLE;

        ImGuiIO.Fonts->SetTexID(nullptr);
    }

    if (Backend->FontImage.IsValid())
    {
        VmaAllocator const &Allocator = GetAllocator();
        Backend->FontImage.DestroyResources(Allocator);
    }
}

bool RenderCore::ImGuiVulkanInit(ImGuiVulkanInitInfo const &VulkanInfo)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    ImGuiIO &ImGuiIO = ImGui::GetIO();

    auto *Backend                   = IM_NEW(ImGuiVulkanData)();
    ImGuiIO.BackendRendererUserData = static_cast<void *>(Backend);
    ImGuiIO.BackendRendererName     = "RenderCore_ImGui_Vulkan";
    ImGuiIO.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasViewports;

    Backend->VulkanInitInfo = VulkanInfo;

    ImGuiVulkanCreateDeviceObjects();

    ImGuiViewport *MainViewport    = ImGui::GetMainViewport();
    MainViewport->RendererUserData = IM_NEW(ImGuiVulkanViewportData)();

    auto *ViewportRenderData = static_cast<ImGuiVulkanViewportData *>(MainViewport->RendererUserData);

    if (ImGuiIO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGuiVulkanInitPlatformInterface();
    }

    for (std::uint8_t ImageIt = 0U; ImageIt < g_ImageCount; ++ImageIt)
    {
        ImGuiVulkanSecondaryCommands &SecondaryCommands = ViewportRenderData->Window.SecondaryCommands.at(ImageIt);
        SecondaryCommands.Initialize(RenderCore::GetLogicalDevice(), RenderCore::GetGraphicsQueue().first);
    }

    return true;
}

void RenderCore::ImGuiVulkanShutdown()
{
    EASY_FUNCTION(profiler::colors::Blue50);

    ImGuiVulkanData *Backend = ImGuiVulkanGetBackendData();
    ImGuiIO &        ImGuiIO = ImGui::GetIO();

    ImGuiVulkanDestroyDeviceObjects();

    ImGuiViewport *MainViewport = ImGui::GetMainViewport();

    auto *ViewportRenderData = static_cast<ImGuiVulkanViewportData *>(MainViewport->RendererUserData);

    VkDevice const &LogicalDevice = RenderCore::GetLogicalDevice();
    for (std::uint8_t ImageIt = 0U; ImageIt < g_ImageCount; ++ImageIt)
    {
        ImGuiVulkanSecondaryCommands &SecondaryCommands = ViewportRenderData->Window.SecondaryCommands.at(ImageIt);
        SecondaryCommands.Destroy(LogicalDevice);
    }

    if (auto *ViewportData = static_cast<ImGuiVulkanViewportData *>(MainViewport->RendererUserData))
    {
        IM_DELETE(ViewportData);
    }

    MainViewport->RendererUserData = nullptr;

    ImGuiVulkanShutdownPlatformInterface();

    ImGuiIO.BackendRendererName     = nullptr;
    ImGuiIO.BackendRendererUserData = nullptr;
    ImGuiIO.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasViewports);

    IM_DELETE(Backend);
}

void RenderCore::ImGuiVulkanNewFrame()
{
    EASY_FUNCTION(profiler::colors::Blue50);

    if (ImGuiVulkanData const *Backend = ImGuiVulkanGetBackendData();
        !Backend->FontDescriptorSet)
    {
        ImGuiVulkanCreateFontsTexture();
    }
}

VkDescriptorSet RenderCore::ImGuiVulkanAddTexture(VkSampler const Sampler, VkImageView const ImageView, VkImageLayout const ImageLayout)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    if (ImageView == VK_NULL_HANDLE)
    {
        return VK_NULL_HANDLE;
    }

    VkDescriptorSet Output;

    ImGuiVulkanData const *    Backend    = ImGuiVulkanGetBackendData();
    ImGuiVulkanInitInfo const &VulkanInfo = Backend->VulkanInitInfo;

    VkDevice const &LogicalDevice = GetLogicalDevice();

    VkDescriptorSetAllocateInfo const AllocationInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = VulkanInfo.DescriptorPool,
            .descriptorSetCount = 1U,
            .pSetLayouts = &Backend->DescriptorSetLayout
    };

    CheckVulkanResult(vkAllocateDescriptorSets(LogicalDevice, &AllocationInfo, &Output));

    VkDescriptorImageInfo const ImageDescriptor { .sampler = Sampler, .imageView = ImageView, .imageLayout = ImageLayout };

    VkWriteDescriptorSet const WriteDescriptor {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = Output,
            .descriptorCount = 1U,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &ImageDescriptor
    };

    vkUpdateDescriptorSets(LogicalDevice, 1U, &WriteDescriptor, 0U, nullptr);

    return Output;
}

void RenderCore::ImGuiVulkanRemoveTexture(VkDescriptorSet const DescriptorSet)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    VkDevice const &           LogicalDevice = GetLogicalDevice();
    ImGuiVulkanData const *    Backend       = ImGuiVulkanGetBackendData();
    ImGuiVulkanInitInfo const &VulkanInfo    = Backend->VulkanInitInfo;
    vkFreeDescriptorSets(LogicalDevice, VulkanInfo.DescriptorPool, 1U, &DescriptorSet);
}

void RenderCore::ImGuiVulkanCreateOrResizeWindow(ImGuiVulkanWindow &WindowData, std::int32_t const Width, std::int32_t const Height)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    VkDevice const &LogicalDevice = GetLogicalDevice();
    vkDeviceWaitIdle(LogicalDevice);

    ImGuiVulkanCreateWindowSwapChain(WindowData, Width, Height);
    ImGuiVulkanCreateWindowCommandBuffers(WindowData);
}

void RenderCore::ImGuiVulkanDestroyWindow(ImGuiVulkanWindow &WindowData)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    VkDevice const &LogicalDevice = GetLogicalDevice();
    vkDeviceWaitIdle(LogicalDevice);

    for (std::uint32_t Iterator = 0U; Iterator < g_ImageCount; Iterator++)
    {
        ImGuiVulkanDestroyFrame(WindowData.Frames.at(Iterator), WindowData.SecondaryCommands.at(Iterator));
    }

    for (std::uint32_t Iterator = 0U; Iterator < static_cast<std::uint32_t>(std::size(WindowData.FrameSemaphores)); Iterator++)
    {
        ImGuiVulkanDestroyFrameSemaphores(WindowData.FrameSemaphores.at(Iterator));
    }

    vkDestroySwapchainKHR(LogicalDevice, WindowData.Swapchain, nullptr);
    vkDestroySurfaceKHR(GetInstance(), WindowData.Surface, nullptr);
}

void RenderCore::ImGuiVulkanResetThreadResources(std::uint8_t const ImageIndex)
{
    EASY_FUNCTION(profiler::colors::Blue50);

    ImGuiViewport const *MainViewport = ImGui::GetMainViewport();

    auto *ViewportRenderData = static_cast<ImGuiVulkanViewportData *>(MainViewport->RendererUserData);

    VkDevice const &LogicalDevice = GetLogicalDevice();
    ViewportRenderData->Window.SecondaryCommands.at(ImageIndex).Reset(LogicalDevice);
}
