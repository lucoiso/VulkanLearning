// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

// Adapted from: https://github.com/ocornut/imgui/blob/docking/backends/imgui_impl_vulkan.h

module;

#include <algorithm>
#include <array>
#include <cstdint>
#include <imgui.h>
#include <vector>
#include <Volk/volk.h>

module RenderCore.Integrations.ImGuiVulkanBackend;

import RenderCore.Utils.Constants;
import RenderCore.Utils.Helpers;
import RenderCore.Runtime.Device;
import RenderCore.Runtime.Instance;
import RenderCore.Runtime.Pipeline;

using namespace RenderCore;

constexpr std::uint64_t g_MinAllocationSize = 1048576U;

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
}

struct ImGuiVulkanFrame
{
    VkCommandPool   CommandPool;
    VkCommandBuffer CommandBuffer;
    VkFence         Fence;
    VkImage         Backbuffer;
    VkImageView     BackbufferView;
};

struct ImGuiVulkanFrameSemaphores
{
    VkSemaphore ImageAcquiredSemaphore;
    VkSemaphore RenderCompleteSemaphore;
};

struct ImGuiVulkanWindow
{
    std::int32_t                Width {};
    std::int32_t                Height {};
    VkSwapchainKHR              Swapchain {};
    VkSurfaceKHR                Surface {};
    VkSurfaceFormatKHR          SurfaceFormat {};
    VkPresentModeKHR            PresentMode;
    VkPipeline                  Pipeline {};
    bool                        ClearEnable;
    VkClearValue                ClearValue {};
    std::uint32_t               FrameIndex {};
    std::uint32_t               SemaphoreCount {};
    std::uint32_t               SemaphoreIndex {};
    ImGuiVulkanFrame *          Frames {};
    ImGuiVulkanFrameSemaphores *FrameSemaphores {};

    ImGuiVulkanWindow()
    {
        memset(this, 0U, sizeof(*this));
        PresentMode = static_cast<VkPresentModeKHR>(~0U);
        ClearEnable = true;
    }
};

struct ImGuiVulkanFrameRenderBuffers
{
    VkDeviceMemory VertexBufferMemory;
    VkDeviceMemory IndexBufferMemory;
    VkDeviceSize   VertexBufferSize;
    VkDeviceSize   IndexBufferSize;
    VkBuffer       VertexBuffer;
    VkBuffer       IndexBuffer;
};

struct ImGuiVulkanWindowRenderBuffers
{
    std::uint32_t                  Index;
    std::uint32_t                  Count;
    ImGuiVulkanFrameRenderBuffers *FrameRenderBuffers;
};

struct ImGuiVulkanViewportData
{
    bool                           WindowOwned;
    ImGuiVulkanWindow              Window {};
    ImGuiVulkanWindowRenderBuffers RenderBuffers {};

    ImGuiVulkanViewportData()
    {
        WindowOwned = false;
        memset(&RenderBuffers, 0U, sizeof(RenderBuffers));
    }

    ~ImGuiVulkanViewportData() = default;
};

struct ImGuiVulkanData
{
    ImGuiVulkanInitInfo   VulkanInitInfo {};
    VkDeviceSize          BufferMemoryAlignment;
    VkDescriptorSetLayout DescriptorSetLayout {};
    VkPipelineLayout      PipelineLayout {};
    VkPipeline            Pipeline {};
    VkShaderModule        ShaderModuleVert {};
    VkShaderModule        ShaderModuleFrag {};

    VkSampler       FontSampler {};
    VkDeviceMemory  FontMemory {};
    VkImage         FontImage {};
    VkImageView     FontView {};
    VkDescriptorSet FontDescriptorSet {};
    VkCommandPool   FontCommandPool {};
    VkCommandBuffer FontCommandBuffer {};

    ImGuiVulkanWindowRenderBuffers MainWindowRenderBuffers {};

    ImGuiVulkanData()
    {
        memset(this, 0U, sizeof(*this));
        BufferMemoryAlignment = 256U;
    }
};

ImGuiVulkanData *ImGuiVulkanGetBackendData()
{
    return ImGui::GetCurrentContext() ? static_cast<ImGuiVulkanData *>(ImGui::GetIO().BackendRendererUserData) : nullptr;
}

std::uint32_t ImGuiVulkanMemoryType(VkMemoryPropertyFlags const Properties, std::uint32_t const Type)
{
    VkPhysicalDeviceMemoryProperties MemProperties;
    vkGetPhysicalDeviceMemoryProperties(GetPhysicalDevice(), &MemProperties);

    for (std::uint32_t Iterator = 0U; Iterator < MemProperties.memoryTypeCount; Iterator++)
    {
        if ((MemProperties.memoryTypes[Iterator].propertyFlags & Properties) == Properties && Type & 1 << Iterator)
        {
            return Iterator;
        }
    }

    return 0xFFFFFFFF;
}

VkDeviceSize AlignBufferSize(VkDeviceSize const Size, VkDeviceSize const Alignment)
{
    return Size + Alignment - 1U & ~(Alignment - 1U);
}

void CreateOrResizeBuffer(VkBuffer &               Buffer,
                          VkDeviceMemory &         BufferMemory,
                          VkDeviceSize &           BufferSize,
                          std::size_t const        NewSize,
                          VkBufferUsageFlags const BufferUsage)
{
    ImGuiVulkanData *Backend       = ImGuiVulkanGetBackendData();
    VkDevice const & LogicalDevice = GetLogicalDevice();

    if (Buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(LogicalDevice, Buffer, nullptr);
    }

    if (BufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(LogicalDevice, BufferMemory, nullptr);
    }

    VkDeviceSize const BufferAlignedSize = AlignBufferSize(std::clamp(NewSize, g_MinAllocationSize, UINT64_MAX), Backend->BufferMemoryAlignment);

    VkBufferCreateInfo const BufferInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = BufferAlignedSize,
            .usage = BufferUsage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    CheckVulkanResult(vkCreateBuffer(LogicalDevice, &BufferInfo, nullptr, &Buffer));

    VkMemoryRequirements Requirements;
    vkGetBufferMemoryRequirements(LogicalDevice, Buffer, &Requirements);
    Backend->BufferMemoryAlignment = Backend->BufferMemoryAlignment > Requirements.alignment
                                         ? Backend->BufferMemoryAlignment
                                         : Requirements.alignment;

    VkMemoryAllocateInfo const AllocationInfo {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = Requirements.size,
            .memoryTypeIndex = ImGuiVulkanMemoryType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, Requirements.memoryTypeBits)
    };
    CheckVulkanResult(vkAllocateMemory(LogicalDevice, &AllocationInfo, nullptr, &BufferMemory));
    CheckVulkanResult(vkBindBufferMemory(LogicalDevice, Buffer, BufferMemory, 0U));

    BufferSize = BufferAlignedSize;
}

void ImGuiVulkanSetupRenderState(ImDrawData const *                   DrawData,
                                 VkPipeline const                     Pipeline,
                                 VkCommandBuffer const                CommandBuffer,
                                 ImGuiVulkanFrameRenderBuffers const *RenderBuffers,
                                 std::int32_t const                   FrameWidth,
                                 std::int32_t const                   FrameHeight)
{
    ImGuiVulkanData const *Backend = ImGuiVulkanGetBackendData();

    vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);

    if (DrawData->TotalVtxCount > 0)
    {
        constexpr VkDeviceSize VertexOffset = 0U;
        vkCmdBindVertexBuffers(CommandBuffer, 0U, 1U, &RenderBuffers->VertexBuffer, &VertexOffset);
        vkCmdBindIndexBuffer(CommandBuffer, RenderBuffers->IndexBuffer, 0U, sizeof(ImDrawIdx) == 2U ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
    }

    {
        VkViewport const Viewport {
                .x = 0.F,
                .y = 0.F,
                .width = static_cast<float>(FrameWidth),
                .height = static_cast<float>(FrameHeight),
                .minDepth = 0.F,
                .maxDepth = 1.F
        };
        vkCmdSetViewport(CommandBuffer, 0U, 1U, &Viewport);
    }

    {
        float const ScaleX = 2.F / DrawData->DisplaySize.x;
        float const ScaleY = 2.F / DrawData->DisplaySize.y;

        constexpr std::uint32_t NumData = 4U;
        std::array const        ConstantData { ScaleX, ScaleY, -1.F - DrawData->DisplayPos.x * ScaleX, -1.F - DrawData->DisplayPos.y * ScaleY };
        vkCmdPushConstants(CommandBuffer, Backend->PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0U, NumData * sizeof(float), std::data(ConstantData));
    }
}

void ImGuiVulkanCreateShaderModules()
{
    ImGuiVulkanData *Backend       = ImGuiVulkanGetBackendData();
    VkDevice const & LogicalDevice = GetLogicalDevice();

    if (Backend->ShaderModuleVert == VK_NULL_HANDLE)
    {
        constexpr VkShaderModuleCreateInfo VertexShaderInfo {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .codeSize = std::size(ShaderData::g_ImGuiVertexShaderBin) * sizeof(std::uint32_t),
                .pCode = std::data(ShaderData::g_ImGuiVertexShaderBin)
        };
        CheckVulkanResult(vkCreateShaderModule(LogicalDevice, &VertexShaderInfo, nullptr, &Backend->ShaderModuleVert));
    }
    if (Backend->ShaderModuleFrag == VK_NULL_HANDLE)
    {
        constexpr VkShaderModuleCreateInfo FragmentShaderInfo {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .codeSize = std::size(ShaderData::g_ImGuiFragmentShaderBin) * sizeof(std::uint32_t),
                .pCode = std::data(ShaderData::g_ImGuiFragmentShaderBin)
        };
        CheckVulkanResult(vkCreateShaderModule(LogicalDevice, &FragmentShaderInfo, nullptr, &Backend->ShaderModuleFrag));
    }
}

void ImGuiVulkanCreatePipeline(VkPipelineCache PipelineCache, VkPipeline *Pipeline)
{
    ImGuiVulkanData *Backend       = ImGuiVulkanGetBackendData();
    VkDevice const & LogicalDevice = GetLogicalDevice();

    ImGuiVulkanCreateShaderModules();

    std::array const ShaderInfos {
            VkPipelineShaderStageCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                    .module = Backend->ShaderModuleVert,
                    .pName = "main"
            },
            VkPipelineShaderStageCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .module = Backend->ShaderModuleFrag,
                    .pName = "main"
            }
    };

    constexpr VkVertexInputBindingDescription VertexBindingDescription {
            .binding = 0U,
            .stride = sizeof(ImDrawVert),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    std::array const AttributeDescriptions {
            VkVertexInputAttributeDescription {
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
    };

    VkPipelineVertexInputStateCreateInfo const VertexInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 1U,
            .pVertexBindingDescriptions = &VertexBindingDescription,
            .vertexAttributeDescriptionCount = static_cast<std::uint32_t>(std::size(AttributeDescriptions)),
            .pVertexAttributeDescriptions = std::data(AttributeDescriptions)
    };

    constexpr VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    constexpr VkPipelineViewportStateCreateInfo ViewportInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1U,
            .scissorCount = 1U
    };

    constexpr VkPipelineRasterizationStateCreateInfo RasterInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .lineWidth = 1.F
    };

    constexpr VkPipelineMultisampleStateCreateInfo MsInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = g_MSAASamples
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

    constexpr VkPipelineDepthStencilStateCreateInfo DepthInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_FALSE,
            .depthWriteEnable = VK_FALSE,
            .depthCompareOp = VK_COMPARE_OP_ALWAYS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE
    };

    VkPipelineColorBlendStateCreateInfo const BlendInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = 1U,
            .pAttachments = &ColorAttachment
    };

    constexpr VkPipelineDynamicStateCreateInfo DynamicStateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = static_cast<std::uint32_t>(std::size(g_DynamicStates)),
            .pDynamicStates = std::data(g_DynamicStates)
    };

    VkGraphicsPipelineCreateInfo const info {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &Backend->VulkanInitInfo.PipelineRenderingCreateInfo,
            .stageCount = static_cast<std::uint32_t>(std::size(ShaderInfos)),
            .pStages = std::data(ShaderInfos),
            .pVertexInputState = &VertexInfo,
            .pInputAssemblyState = &InputAssemblyInfo,
            .pViewportState = &ViewportInfo,
            .pRasterizationState = &RasterInfo,
            .pMultisampleState = &MsInfo,
            .pDepthStencilState = &DepthInfo,
            .pColorBlendState = &BlendInfo,
            .pDynamicState = &DynamicStateInfo,
            .layout = Backend->PipelineLayout
    };

    CheckVulkanResult(vkCreateGraphicsPipelines(LogicalDevice, PipelineCache, 1, &info, nullptr, Pipeline));
}

void ImGuiVulkanCreateWindow(ImGuiViewport *Viewport)
{
    VkInstance const &      Instance       = GetInstance();
    VkPhysicalDevice const &PhysicalDevice = GetPhysicalDevice();

    auto const &[QueueFamilyIndex, Queue] = GetGraphicsQueue();

    ImGuiVulkanData *Backend      = ImGuiVulkanGetBackendData();
    auto *           ViewportData = IM_NEW(ImGuiVulkanViewportData)();

    Viewport->RendererUserData                  = ViewportData;
    ImGuiVulkanWindow *const &       WindowData = &ViewportData->Window;
    ImGuiVulkanInitInfo const *const&VulkanInfo = &Backend->VulkanInitInfo;

    ImGuiPlatformIO const &PlatformIO = ImGui::GetPlatformIO();
    CheckVulkanResult(static_cast<VkResult>(PlatformIO.Platform_CreateVkSurface(Viewport,
                                                                                reinterpret_cast<ImU64>(Instance),
                                                                                static_cast<const void *>(nullptr),
                                                                                reinterpret_cast<ImU64 *>(&WindowData->Surface))));

    VkBool32 Result;
    vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, QueueFamilyIndex, WindowData->Surface, &Result);
    if (Result != VK_TRUE)
    {
        IM_ASSERT(0);
        return;
    }

    ImVector<VkFormat> SurfaceFormats;

    for (std::uint32_t FormatIt = 0U; FormatIt < VulkanInfo->PipelineRenderingCreateInfo.colorAttachmentCount; ++FormatIt)
    {
        SurfaceFormats.push_back(VulkanInfo->PipelineRenderingCreateInfo.pColorAttachmentFormats[FormatIt]);
    }

    for (constexpr std::array DefaultFormats { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
         VkFormat             FormatIt : DefaultFormats)
    {
        SurfaceFormats.push_back(FormatIt);
    }

    constexpr VkColorSpaceKHR RequestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    WindowData->SurfaceFormat                          = ImGuiVulkanSelectSurfaceFormat(WindowData->Surface,
                                                                                        SurfaceFormats.Data,
                                                                                        static_cast<size_t>(SurfaceFormats.Size),
                                                                                        RequestSurfaceColorSpace);

    constexpr std::array PresentModes { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
    WindowData->PresentMode = ImGuiVulkanSelectPresentMode(WindowData->Surface, std::data(PresentModes), std::size(PresentModes));

    WindowData->ClearEnable = Viewport->Flags & ImGuiViewportFlags_NoRendererClear ? false : true;

    ImGuiVulkanCreateOrResizeWindow(WindowData, static_cast<std::int32_t>(Viewport->Size.x), static_cast<std::int32_t>(Viewport->Size.y));
    ViewportData->WindowOwned = true;
}

void ImGuiVulkanDestroyViewport(ImGuiViewport *Viewport)
{
    if (auto *ViewportData = static_cast<ImGuiVulkanViewportData *>(Viewport->RendererUserData))
    {
        if (ViewportData->WindowOwned)
        {
            ImGuiVulkanDestroyWindow(&ViewportData->Window);
        }

        RenderCore::ImGuiVulkanDestroyWindowRenderBuffers(&ViewportData->RenderBuffers);
        IM_DELETE(ViewportData);
    }

    Viewport->RendererUserData = nullptr;
}

void ImGuiVulkanSetWindowSize(ImGuiViewport *Viewport, ImVec2 const Size)
{
    auto *ViewportData = static_cast<ImGuiVulkanViewportData *>(Viewport->RendererUserData);
    if (ViewportData == nullptr)
    {
        return;
    }
    ViewportData->Window.ClearEnable = Viewport->Flags & ImGuiViewportFlags_NoRendererClear ? false : true;

    ImGuiVulkanCreateOrResizeWindow(&ViewportData->Window, static_cast<std::int32_t>(Size.x), static_cast<std::int32_t>(Size.y));
}

void ImGuiVulkanRenderWindow(ImGuiViewport *Viewport, void *)
{
    VkDevice const &LogicalDevice             = GetLogicalDevice();
    auto const &    [QueueFamilyIndex, Queue] = GetGraphicsQueue();

    auto *             ViewportData = static_cast<ImGuiVulkanViewportData *>(Viewport->RendererUserData);
    ImGuiVulkanWindow *WindowData   = &ViewportData->Window;

    ImGuiVulkanFrame *          FrameData       = &WindowData->Frames[WindowData->FrameIndex];
    ImGuiVulkanFrameSemaphores *FrameSemaphores = &WindowData->FrameSemaphores[WindowData->SemaphoreIndex];
    {
        {
            CheckVulkanResult(vkAcquireNextImageKHR(LogicalDevice,
                                                    WindowData->Swapchain,
                                                    g_Timeout,
                                                    FrameSemaphores->ImageAcquiredSemaphore,
                                                    VK_NULL_HANDLE,
                                                    &WindowData->FrameIndex));
            CheckVulkanResult(vkWaitForFences(LogicalDevice, 1U, &FrameData->Fence, VK_TRUE, g_Timeout));

            FrameData = &WindowData->Frames[WindowData->FrameIndex];
        }

        {
            CheckVulkanResult(vkResetCommandPool(LogicalDevice, FrameData->CommandPool, 0U));
            constexpr VkCommandBufferBeginInfo CommandBufferBeginInfo {
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
            };

            CheckVulkanResult(vkBeginCommandBuffer(FrameData->CommandBuffer, &CommandBufferBeginInfo));
        }
        {
            constexpr std::array ClearColor { 0.F, 0.F, 0.F, 1.F };
            memcpy(&WindowData->ClearValue.color.float32[0], std::data(ClearColor), sizeof(ClearColor));
        }

        {
            VkImageMemoryBarrier const ImageMemoryBarrier {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = 0U,
                    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .srcQueueFamilyIndex = QueueFamilyIndex,
                    .dstQueueFamilyIndex = QueueFamilyIndex,
                    .image = FrameData->Backbuffer,
                    .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0U, 1U, 0U, 1U }
            };

            vkCmdPipelineBarrier(FrameData->CommandBuffer,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                 0U,
                                 0U,
                                 nullptr,
                                 0U,
                                 nullptr,
                                 1U,
                                 &ImageMemoryBarrier);
        }

        {
            VkRenderingAttachmentInfo const AttachmentInfo = {
                    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                    .imageView = FrameData->BackbufferView,
                    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .resolveMode = VK_RESOLVE_MODE_NONE,
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue = WindowData->ClearValue
            };

            VkRenderingInfo const RenderingInfo = {
                    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
                    .renderArea = { { 0, 0 }, { static_cast<std::uint32_t>(Viewport->Size.x), static_cast<std::uint32_t>(Viewport->Size.y) } },
                    .layerCount = 1U,
                    .viewMask = 0U,
                    .colorAttachmentCount = 1U,
                    .pColorAttachments = &AttachmentInfo
            };

            vkCmdBeginRendering(FrameData->CommandBuffer, &RenderingInfo);
        }
    }

    ImGuiVulkanRenderDrawData(Viewport->DrawData, FrameData->CommandBuffer);
    vkCmdEndRendering(FrameData->CommandBuffer);

    {
        VkImageMemoryBarrier const ImageMemoryBarrier {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .srcQueueFamilyIndex = QueueFamilyIndex,
                .dstQueueFamilyIndex = QueueFamilyIndex,
                .image = FrameData->Backbuffer,
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0U, 1U, 0U, 1U }
        };

        vkCmdPipelineBarrier(FrameData->CommandBuffer,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                             0U,
                             0U,
                             nullptr,
                             0U,
                             nullptr,
                             1U,
                             &ImageMemoryBarrier);

        {
            constexpr VkPipelineStageFlags WaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            VkSubmitInfo const             SubmitInfo {
                    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                    .waitSemaphoreCount = 1U,
                    .pWaitSemaphores = &FrameSemaphores->ImageAcquiredSemaphore,
                    .pWaitDstStageMask = &WaitStage,
                    .commandBufferCount = 1U,
                    .pCommandBuffers = &FrameData->CommandBuffer,
                    .signalSemaphoreCount = 1U,
                    .pSignalSemaphores = &FrameSemaphores->RenderCompleteSemaphore
            };

            CheckVulkanResult(vkEndCommandBuffer(FrameData->CommandBuffer));
            CheckVulkanResult(vkResetFences(LogicalDevice, 1U, &FrameData->Fence));
            CheckVulkanResult(vkQueueSubmit(Queue, 1U, &SubmitInfo, FrameData->Fence));
        }
    }
}

void ImGuiVulkanSwapBuffers(ImGuiViewport *Viewport, void *)
{
    auto const &[QueueFamilyIndex, Queue] = GetGraphicsQueue();

    auto *                    ViewportData = static_cast<ImGuiVulkanViewportData *>(Viewport->RendererUserData);
    ImGuiVulkanWindow *const &WindowData   = &ViewportData->Window;

    std::uint32_t const                     FrameIndex      = WindowData->FrameIndex;
    ImGuiVulkanFrameSemaphores const *const&FrameSemaphores = &WindowData->FrameSemaphores[WindowData->SemaphoreIndex];

    VkPresentInfoKHR const PresentInfo {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1U,
            .pWaitSemaphores = &FrameSemaphores->RenderCompleteSemaphore,
            .swapchainCount = 1U,
            .pSwapchains = &WindowData->Swapchain,
            .pImageIndices = &FrameIndex
    };

    if (VkResult const Result = vkQueuePresentKHR(Queue, &PresentInfo);
        Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR)
    {
        ImGuiVulkanCreateOrResizeWindow(&ViewportData->Window,
                                        static_cast<std::int32_t>(Viewport->Size.x),
                                        static_cast<std::int32_t>(Viewport->Size.y));
    }

    WindowData->FrameIndex     = (WindowData->FrameIndex + 1) % g_MinImageCount;
    WindowData->SemaphoreIndex = (WindowData->SemaphoreIndex + 1) % WindowData->SemaphoreCount;
}

bool RenderCore::ImGuiVulkanCreateDeviceObjects()
{
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

    if (!Backend->PipelineLayout)
    {
        constexpr VkPushConstantRange PushConstantRange { .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = 0U, .size = sizeof(float) * 4U };

        VkDescriptorSetLayout const SetLayout { Backend->DescriptorSetLayout };

        VkPipelineLayoutCreateInfo const LayoutInfo {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = 1U,
                .pSetLayouts = &SetLayout,
                .pushConstantRangeCount = 1U,
                .pPushConstantRanges = &PushConstantRange
        };

        CheckVulkanResult(vkCreatePipelineLayout(LogicalDevice, &LayoutInfo, nullptr, &Backend->PipelineLayout));
    }

    ImGuiVulkanCreatePipeline(GetPipelineCache(), &Backend->Pipeline);
    return true;
}

void RenderCore::ImGuiVulkanDestroyDeviceObjects()
{
    ImGuiVulkanData *Backend       = ImGuiVulkanGetBackendData();
    VkDevice const & LogicalDevice = GetLogicalDevice();

    ImGuiVulkanDestroyAllViewportsRenderBuffers();
    ImGuiVulkanDestroyFontsTexture();

    if (Backend->FontCommandBuffer)
    {
        vkFreeCommandBuffers(LogicalDevice, Backend->FontCommandPool, 1, &Backend->FontCommandBuffer);
        Backend->FontCommandBuffer = VK_NULL_HANDLE;
    }

    if (Backend->FontCommandPool)
    {
        vkDestroyCommandPool(LogicalDevice, Backend->FontCommandPool, nullptr);
        Backend->FontCommandPool = VK_NULL_HANDLE;
    }

    if (Backend->ShaderModuleVert)
    {
        vkDestroyShaderModule(LogicalDevice, Backend->ShaderModuleVert, nullptr);
        Backend->ShaderModuleVert = VK_NULL_HANDLE;
    }

    if (Backend->ShaderModuleFrag)
    {
        vkDestroyShaderModule(LogicalDevice, Backend->ShaderModuleFrag, nullptr);
        Backend->ShaderModuleFrag = VK_NULL_HANDLE;
    }

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

    if (Backend->PipelineLayout)
    {
        vkDestroyPipelineLayout(LogicalDevice, Backend->PipelineLayout, nullptr);
        Backend->PipelineLayout = VK_NULL_HANDLE;
    }

    if (Backend->Pipeline)
    {
        vkDestroyPipeline(LogicalDevice, Backend->Pipeline, nullptr);
        Backend->Pipeline = VK_NULL_HANDLE;
    }
}

void RenderCore::ImGuiVulkanDestroyFrameRenderBuffers(ImGuiVulkanFrameRenderBuffers *FrameBuffers)
{
    VkDevice const &LogicalDevice = GetLogicalDevice();

    if (FrameBuffers->VertexBuffer)
    {
        vkDestroyBuffer(LogicalDevice, FrameBuffers->VertexBuffer, nullptr);
        FrameBuffers->VertexBuffer = VK_NULL_HANDLE;
    }

    if (FrameBuffers->VertexBufferMemory)
    {
        vkFreeMemory(LogicalDevice, FrameBuffers->VertexBufferMemory, nullptr);
        FrameBuffers->VertexBufferMemory = VK_NULL_HANDLE;
    }

    if (FrameBuffers->IndexBuffer)
    {
        vkDestroyBuffer(LogicalDevice, FrameBuffers->IndexBuffer, nullptr);
        FrameBuffers->IndexBuffer = VK_NULL_HANDLE;
    }

    if (FrameBuffers->IndexBufferMemory)
    {
        vkFreeMemory(LogicalDevice, FrameBuffers->IndexBufferMemory, nullptr);
        FrameBuffers->IndexBufferMemory = VK_NULL_HANDLE;
    }

    FrameBuffers->VertexBufferSize = 0;
    FrameBuffers->IndexBufferSize  = 0;
}

void RenderCore::ImGuiVulkanDestroyWindowRenderBuffers(ImGuiVulkanWindowRenderBuffers *Buffers)
{
    for (std::uint32_t BufferIt = 0U; BufferIt < Buffers->Count; ++BufferIt)
    {
        RenderCore::ImGuiVulkanDestroyFrameRenderBuffers(&Buffers->FrameRenderBuffers[BufferIt]);
    }

    IM_FREE(Buffers->FrameRenderBuffers);
    Buffers->FrameRenderBuffers = nullptr;
    Buffers->Index              = 0U;
    Buffers->Count              = 0U;
}

void RenderCore::ImGuiVulkanCreateWindowCommandBuffers(ImGuiVulkanWindow const *WindowData)
{
    VkDevice const &LogicalDevice             = GetLogicalDevice();
    auto const &    [QueueFamilyIndex, Queue] = GetGraphicsQueue();

    for (std::uint32_t Iterator = 0U; Iterator < g_MinImageCount; Iterator++)
    {
        ImGuiVulkanFrame *const &FrameData = &WindowData->Frames[Iterator];
        {
            VkCommandPoolCreateInfo const CommandPoolInfo {
                    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                    .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                    .queueFamilyIndex = QueueFamilyIndex
            };

            CheckVulkanResult(vkCreateCommandPool(LogicalDevice, &CommandPoolInfo, nullptr, &FrameData->CommandPool));
        }

        {
            VkCommandBufferAllocateInfo const CommandBufferInfo {
                    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                    .commandPool = FrameData->CommandPool,
                    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                    .commandBufferCount = 1U
            };

            CheckVulkanResult(vkAllocateCommandBuffers(LogicalDevice, &CommandBufferInfo, &FrameData->CommandBuffer));
        }

        {
            constexpr VkFenceCreateInfo FenceInfo { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT };

            CheckVulkanResult(vkCreateFence(LogicalDevice, &FenceInfo, nullptr, &FrameData->Fence));
        }
    }

    for (std::uint32_t Iterator = 0U; Iterator < WindowData->SemaphoreCount; Iterator++)
    {
        ImGuiVulkanFrameSemaphores *const &FrameSemaphore = &WindowData->FrameSemaphores[Iterator];

        constexpr VkSemaphoreCreateInfo SemaphoreInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

        CheckVulkanResult(vkCreateSemaphore(LogicalDevice, &SemaphoreInfo, nullptr, &FrameSemaphore->ImageAcquiredSemaphore));
        CheckVulkanResult(vkCreateSemaphore(LogicalDevice, &SemaphoreInfo, nullptr, &FrameSemaphore->RenderCompleteSemaphore));
    }
}

void RenderCore::ImGuiVulkanCreateWindowSwapChain(ImGuiVulkanWindow *WindowData, std::int32_t WindowWidth, std::int32_t WindowHeight)
{
    VkPhysicalDevice const &PhysicalDevice = GetPhysicalDevice();
    VkDevice const &        LogicalDevice  = GetLogicalDevice();

    VkSwapchainKHR const OldSwapchain = WindowData->Swapchain;
    WindowData->Swapchain             = VK_NULL_HANDLE;
    CheckVulkanResult(vkDeviceWaitIdle(LogicalDevice));

    for (std::uint32_t Iterator = 0U; Iterator < g_MinImageCount; Iterator++)
    {
        ImGuiVulkanDestroyFrame(&WindowData->Frames[Iterator]);
    }

    for (std::uint32_t Iterator = 0U; Iterator < WindowData->SemaphoreCount; Iterator++)
    {
        ImGuiVulkanDestroyFrameSemaphores(&WindowData->FrameSemaphores[Iterator]);
    }

    IM_FREE(WindowData->Frames);
    IM_FREE(WindowData->FrameSemaphores);

    WindowData->Frames          = nullptr;
    WindowData->FrameSemaphores = nullptr;

    if (WindowData->Pipeline)
    {
        vkDestroyPipeline(LogicalDevice, WindowData->Pipeline, nullptr);
    }

    {
        VkSwapchainCreateInfoKHR SwapchainCreateInfo {
                .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .surface = WindowData->Surface,
                .minImageCount = g_MinImageCount,
                .imageFormat = WindowData->SurfaceFormat.format,
                .imageColorSpace = WindowData->SurfaceFormat.colorSpace,
                .imageArrayLayers = 1,
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                .presentMode = WindowData->PresentMode,
                .clipped = VK_TRUE,
                .oldSwapchain = OldSwapchain
        };

        VkSurfaceCapabilitiesKHR SurfaceCapabilities;
        CheckVulkanResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, WindowData->Surface, &SurfaceCapabilities));

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
            SwapchainCreateInfo.imageExtent.width  = WindowData->Width  = WindowWidth;
            SwapchainCreateInfo.imageExtent.height = WindowData->Height = WindowHeight;
        }
        else
        {
            SwapchainCreateInfo.imageExtent.width  = WindowData->Width  = SurfaceCapabilities.currentExtent.width;
            SwapchainCreateInfo.imageExtent.height = WindowData->Height = SurfaceCapabilities.currentExtent.height;
        }

        std::uint32_t Count = 0U;
        CheckVulkanResult(vkGetSwapchainImagesKHR(LogicalDevice, WindowData->Swapchain, &Count, nullptr));

        std::vector<VkImage> BackBuffers(Count, VK_NULL_HANDLE);
        CheckVulkanResult(vkGetSwapchainImagesKHR(LogicalDevice, WindowData->Swapchain, &Count, std::data(BackBuffers)));

        WindowData->SemaphoreCount  = g_MinImageCount + 1U;
        WindowData->Frames          = static_cast<ImGuiVulkanFrame *>(IM_ALLOC(sizeof(ImGuiVulkanFrame) * g_MinImageCount));
        WindowData->FrameSemaphores = static_cast<ImGuiVulkanFrameSemaphores *>(
            IM_ALLOC(sizeof(ImGuiVulkanFrameSemaphores) * WindowData->SemaphoreCount));

        memset(WindowData->Frames, 0U, sizeof(WindowData->Frames[0]) * g_MinImageCount);
        memset(WindowData->FrameSemaphores, 0U, sizeof(WindowData->FrameSemaphores[0]) * WindowData->SemaphoreCount);

        for (std::uint32_t Iterator = 0U; Iterator < g_MinImageCount; Iterator++)
        {
            WindowData->Frames[Iterator].Backbuffer = BackBuffers[Iterator];
        }
    }

    if (OldSwapchain)
    {
        vkDestroySwapchainKHR(LogicalDevice, OldSwapchain, nullptr);
    }

    {
        VkImageViewCreateInfo ImageViewCreate {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = WindowData->SurfaceFormat.format,
                .components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0U, 1U, 0U, 1U }
        };

        for (std::uint32_t Iterator = 0U; Iterator < g_MinImageCount; Iterator++)
        {
            ImGuiVulkanFrame *const&FrameData = &WindowData->Frames[Iterator];
            ImageViewCreate.image             = FrameData->Backbuffer;
            CheckVulkanResult(vkCreateImageView(LogicalDevice, &ImageViewCreate, nullptr, &FrameData->BackbufferView));
        }
    }
}

void RenderCore::ImGuiVulkanDestroyFrame(ImGuiVulkanFrame *FrameData)
{
    VkDevice const &LogicalDevice = GetLogicalDevice();

    vkDestroyFence(LogicalDevice, FrameData->Fence, nullptr);
    vkFreeCommandBuffers(LogicalDevice, FrameData->CommandPool, 1, &FrameData->CommandBuffer);
    vkDestroyCommandPool(LogicalDevice, FrameData->CommandPool, nullptr);

    FrameData->Fence         = VK_NULL_HANDLE;
    FrameData->CommandBuffer = VK_NULL_HANDLE;
    FrameData->CommandPool   = VK_NULL_HANDLE;

    vkDestroyImageView(LogicalDevice, FrameData->BackbufferView, nullptr);
}

void RenderCore::ImGuiVulkanDestroyFrameSemaphores(ImGuiVulkanFrameSemaphores *FrameSemaphore)
{
    VkDevice const &LogicalDevice = GetLogicalDevice();

    vkDestroySemaphore(LogicalDevice, FrameSemaphore->ImageAcquiredSemaphore, nullptr);
    vkDestroySemaphore(LogicalDevice, FrameSemaphore->RenderCompleteSemaphore, nullptr);

    FrameSemaphore->ImageAcquiredSemaphore  = VK_NULL_HANDLE;
    FrameSemaphore->RenderCompleteSemaphore = VK_NULL_HANDLE;
}

void RenderCore::ImGuiVulkanDestroyAllViewportsRenderBuffers()
{
    ImGuiPlatformIO &PlatformIO = ImGui::GetPlatformIO();

    for (std::uint32_t ViewportIt = 0U; ViewportIt < PlatformIO.Viewports.Size; ++ViewportIt)
    {
        if (auto *ViewportData = static_cast<ImGuiVulkanViewportData *>(PlatformIO.Viewports[ViewportIt]->RendererUserData))
        {
            RenderCore::ImGuiVulkanDestroyWindowRenderBuffers(&ViewportData->RenderBuffers);
        }
    }
}

void RenderCore::ImGuiVulkanInitPlatformInterface()
{
    ImGuiPlatformIO &PlatformIO       = ImGui::GetPlatformIO();
    PlatformIO.Renderer_CreateWindow  = ImGuiVulkanCreateWindow;
    PlatformIO.Renderer_DestroyWindow = ImGuiVulkanDestroyViewport;
    PlatformIO.Renderer_SetWindowSize = ImGuiVulkanSetWindowSize;
    PlatformIO.Renderer_RenderWindow  = ImGuiVulkanRenderWindow;
    PlatformIO.Renderer_SwapBuffers   = ImGuiVulkanSwapBuffers;
}

void RenderCore::ImGuiVulkanShutdownPlatformInterface()
{
    ImGui::DestroyPlatformWindows();
}

void RenderCore::ImGuiVulkanRenderDrawData(ImDrawData *DrawData, VkCommandBuffer const CommandBuffer)
{
    std::uint32_t const FrameWidth  = static_cast<std::uint32_t>(DrawData->DisplaySize.x * DrawData->FramebufferScale.x);
    std::uint32_t const FrameHeight = static_cast<std::uint32_t>(DrawData->DisplaySize.y * DrawData->FramebufferScale.y);

    if (FrameWidth <= 0U || FrameHeight <= 0U)
    {
        return;
    }

    ImGuiVulkanData const *Backend = ImGuiVulkanGetBackendData();

    auto *                          ViewportRenderData  = static_cast<ImGuiVulkanViewportData *>(DrawData->OwnerViewport->RendererUserData);
    ImGuiVulkanWindowRenderBuffers *WindowRenderBuffers = &ViewportRenderData->RenderBuffers;

    if (WindowRenderBuffers->FrameRenderBuffers == nullptr)
    {
        WindowRenderBuffers->Index              = 0U;
        WindowRenderBuffers->Count              = g_MinImageCount;
        WindowRenderBuffers->FrameRenderBuffers = static_cast<ImGuiVulkanFrameRenderBuffers *>(
            IM_ALLOC(sizeof(ImGuiVulkanFrameRenderBuffers) * WindowRenderBuffers->Count));
        memset(WindowRenderBuffers->FrameRenderBuffers, 0U, sizeof(ImGuiVulkanFrameRenderBuffers) * WindowRenderBuffers->Count);
    }

    WindowRenderBuffers->Index                   = (WindowRenderBuffers->Index + 1) % WindowRenderBuffers->Count;
    ImGuiVulkanFrameRenderBuffers *RenderBuffers = &WindowRenderBuffers->FrameRenderBuffers[WindowRenderBuffers->Index];

    if (DrawData->TotalVtxCount > 0U)
    {
        std::size_t const VertexSize = AlignBufferSize(DrawData->TotalVtxCount * sizeof(ImDrawVert), Backend->BufferMemoryAlignment);
        std::size_t const IndexSize  = AlignBufferSize(DrawData->TotalIdxCount * sizeof(ImDrawIdx), Backend->BufferMemoryAlignment);

        if (RenderBuffers->VertexBuffer == VK_NULL_HANDLE || RenderBuffers->VertexBufferSize < VertexSize)
        {
            CreateOrResizeBuffer(RenderBuffers->VertexBuffer,
                                 RenderBuffers->VertexBufferMemory,
                                 RenderBuffers->VertexBufferSize,
                                 VertexSize,
                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        }

        if (RenderBuffers->IndexBuffer == VK_NULL_HANDLE || RenderBuffers->IndexBufferSize < IndexSize)
        {
            CreateOrResizeBuffer(RenderBuffers->IndexBuffer,
                                 RenderBuffers->IndexBufferMemory,
                                 RenderBuffers->IndexBufferSize,
                                 IndexSize,
                                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        }

        ImDrawVert *VertexBuffer = nullptr;
        ImDrawIdx * IndexBuffer  = nullptr;

        VkDevice const &LogicalDevice = GetLogicalDevice();

        CheckVulkanResult(vkMapMemory(LogicalDevice,
                                      RenderBuffers->VertexBufferMemory,
                                      0U,
                                      VertexSize,
                                      0U,
                                      reinterpret_cast<void **>(&VertexBuffer)));
        CheckVulkanResult(vkMapMemory(LogicalDevice, RenderBuffers->IndexBufferMemory, 0U, IndexSize, 0U, reinterpret_cast<void **>(&IndexBuffer)));

        for (std::int32_t CmdIt = 0; CmdIt < DrawData->CmdListsCount; ++CmdIt)
        {
            const ImDrawList *const&Commands = DrawData->CmdLists[CmdIt];
            memcpy(VertexBuffer, Commands->VtxBuffer.Data, Commands->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(IndexBuffer, Commands->IdxBuffer.Data, Commands->IdxBuffer.Size * sizeof(ImDrawIdx));
            VertexBuffer += Commands->VtxBuffer.Size;
            IndexBuffer += Commands->IdxBuffer.Size;
        }

        constexpr std::uint8_t RangeNum = 2U;
        std::array const       MemoryRange {
                VkMappedMemoryRange {
                        .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                        .memory = RenderBuffers->VertexBufferMemory,
                        .size = VK_WHOLE_SIZE
                },
                VkMappedMemoryRange {
                        .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
                        .memory = RenderBuffers->IndexBufferMemory,
                        .size = VK_WHOLE_SIZE
                }
        };

        CheckVulkanResult(vkFlushMappedMemoryRanges(LogicalDevice, RangeNum, std::data(MemoryRange)));
        vkUnmapMemory(LogicalDevice, RenderBuffers->VertexBufferMemory);
        vkUnmapMemory(LogicalDevice, RenderBuffers->IndexBufferMemory);
    }

    ImGuiVulkanSetupRenderState(DrawData, Backend->Pipeline, CommandBuffer, RenderBuffers, FrameWidth, FrameHeight);

    ImVec2 const &DisplayPosition = DrawData->DisplayPos;
    ImVec2 const &BufferScale     = DrawData->FramebufferScale;

    std::uint32_t GlobalVtxOffset = 0U;
    std::uint32_t GlobalIdxOffset = 0U;

    for (std::uint32_t CmdListIt = 0U; CmdListIt < DrawData->CmdListsCount; ++CmdListIt)
    {
        const ImDrawList *const &Commands = DrawData->CmdLists[CmdListIt];

        for (std::uint32_t CmdIt = 0U; CmdIt < Commands->CmdBuffer.Size; ++CmdIt)
        {
            if (const ImDrawCmd *const&DrawCmd = &Commands->CmdBuffer[CmdIt];
                DrawCmd->UserCallback != nullptr)
            {
                if (DrawCmd->UserCallback == ImDrawCallback_ResetRenderState)
                {
                    ImGuiVulkanSetupRenderState(DrawData, Backend->Pipeline, CommandBuffer, RenderBuffers, FrameWidth, FrameHeight);
                }
                else
                {
                    DrawCmd->UserCallback(Commands, DrawCmd);
                }
            }
            else
            {
                ImVec2 MinClip((DrawCmd->ClipRect.x - DisplayPosition.x) * BufferScale.x, (DrawCmd->ClipRect.y - DisplayPosition.y) * BufferScale.y);
                ImVec2 MaxClip((DrawCmd->ClipRect.z - DisplayPosition.x) * BufferScale.x, (DrawCmd->ClipRect.w - DisplayPosition.y) * BufferScale.y);

                if (MinClip.x < 0.F)
                {
                    MinClip.x = 0.F;
                }
                if (MinClip.y < 0.F)
                {
                    MinClip.y = 0.F;
                }

                if (MaxClip.x > static_cast<float>(FrameWidth))
                {
                    MaxClip.x = static_cast<float>(FrameWidth);
                }

                if (MaxClip.y > static_cast<float>(FrameHeight))
                {
                    MaxClip.y = static_cast<float>(FrameHeight);
                }

                if (MaxClip.x <= MinClip.x || MaxClip.y <= MinClip.y)
                {
                    continue;
                }

                VkRect2D const Scissor {
                        .offset = { .x = static_cast<int32_t>(MinClip.x), .y = static_cast<int32_t>(MinClip.y) },
                        .extent = { .width = static_cast<uint32_t>(MaxClip.x - MinClip.x), .height = static_cast<uint32_t>(MaxClip.y - MinClip.y) }
                };
                vkCmdSetScissor(CommandBuffer, 0U, 1U, &Scissor);

                auto DescriptorSet = static_cast<VkDescriptorSet>(DrawCmd->TextureId);
                if constexpr (sizeof(ImTextureID) < sizeof(ImU64))
                {
                    DescriptorSet = Backend->FontDescriptorSet;
                }

                vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Backend->PipelineLayout, 0U, 1U, &DescriptorSet, 0U, nullptr);

                vkCmdDrawIndexed(CommandBuffer,
                                 DrawCmd->ElemCount,
                                 1U,
                                 DrawCmd->IdxOffset + GlobalIdxOffset,
                                 static_cast<int32_t>(DrawCmd->VtxOffset) + GlobalVtxOffset,
                                 0U);
            }
        }
        GlobalIdxOffset += Commands->IdxBuffer.Size;
        GlobalVtxOffset += Commands->VtxBuffer.Size;
    }

    VkRect2D const Scissor {
            .offset = { .x = 0U, .y = 0U },
            .extent = { .width = static_cast<uint32_t>(FrameWidth), .height = static_cast<uint32_t>(FrameHeight) }
    };
    vkCmdSetScissor(CommandBuffer, 0U, 1U, &Scissor);
}

bool RenderCore::ImGuiVulkanCreateFontsTexture()
{
    ImGuiIO &        ImGuiIO = ImGui::GetIO();
    ImGuiVulkanData *Backend = ImGuiVulkanGetBackendData();

    VkDevice const &LogicalDevice             = GetLogicalDevice();
    auto const &    [QueueFamilyIndex, Queue] = GetGraphicsQueue();

    if (Backend->FontView || Backend->FontImage || Backend->FontMemory || Backend->FontDescriptorSet)
    {
        vkQueueWaitIdle(Queue);
        ImGuiVulkanDestroyFontsTexture();
    }

    if (Backend->FontCommandPool == VK_NULL_HANDLE)
    {
        VkCommandPoolCreateInfo const CommandPoolCreateInfo {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags = 0U,
                .queueFamilyIndex = QueueFamilyIndex
        };
        vkCreateCommandPool(LogicalDevice, &CommandPoolCreateInfo, nullptr, &Backend->FontCommandPool);
    }

    if (Backend->FontCommandBuffer == VK_NULL_HANDLE)
    {
        VkCommandBufferAllocateInfo const CommandBufferAllocateInfo {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = Backend->FontCommandPool,
                .commandBufferCount = 1U
        };
        CheckVulkanResult(vkAllocateCommandBuffers(LogicalDevice, &CommandBufferAllocateInfo, &Backend->FontCommandBuffer));
    }

    CheckVulkanResult(vkResetCommandPool(LogicalDevice, Backend->FontCommandPool, 0));

    {
        constexpr VkCommandBufferBeginInfo CommandBufferBeginInfo {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };

        CheckVulkanResult(vkBeginCommandBuffer(Backend->FontCommandBuffer, &CommandBufferBeginInfo));
    }

    unsigned char *ImageData;
    std::int32_t   ImageWidth;
    std::int32_t   ImageHeight;
    ImGuiIO.Fonts->GetTexDataAsRGBA32(&ImageData, &ImageWidth, &ImageHeight);
    std::size_t const BufferSize = ImageWidth * ImageHeight * 4U * sizeof(char);

    {
        VkImageCreateInfo const ImageCreateInfo {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .extent = { .width = static_cast<std::uint32_t>(ImageWidth), .height = static_cast<std::uint32_t>(ImageHeight), .depth = 1U },
                .mipLevels = 1U,
                .arrayLayers = 1U,
                .samples = g_MSAASamples,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };
        CheckVulkanResult(vkCreateImage(LogicalDevice, &ImageCreateInfo, nullptr, &Backend->FontImage));

        VkMemoryRequirements Requirements;
        vkGetImageMemoryRequirements(LogicalDevice, Backend->FontImage, &Requirements);

        VkMemoryAllocateInfo const AllocationInfo {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = std::clamp(Requirements.size, g_MinAllocationSize, UINT64_MAX),
                .memoryTypeIndex = ImGuiVulkanMemoryType(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, Requirements.memoryTypeBits)
        };
        CheckVulkanResult(vkAllocateMemory(LogicalDevice, &AllocationInfo, nullptr, &Backend->FontMemory));
        CheckVulkanResult(vkBindImageMemory(LogicalDevice, Backend->FontImage, Backend->FontMemory, 0U));
    }

    {
        VkImageViewCreateInfo const ImageViewCreateInfo {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = Backend->FontImage,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1U, .layerCount = 1U }
        };
        CheckVulkanResult(vkCreateImageView(LogicalDevice, &ImageViewCreateInfo, nullptr, &Backend->FontView));
    }

    Backend->FontDescriptorSet = ImGuiVulkanAddTexture(Backend->FontSampler, Backend->FontView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkDeviceMemory UploadBufferMemory;
    VkBuffer       UploadBuffer;

    {
        VkBufferCreateInfo const BufferInfo {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = BufferSize,
                .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };
        CheckVulkanResult(vkCreateBuffer(LogicalDevice, &BufferInfo, nullptr, &UploadBuffer));

        VkMemoryRequirements Requirements;
        vkGetBufferMemoryRequirements(LogicalDevice, UploadBuffer, &Requirements);
        Backend->BufferMemoryAlignment = Backend->BufferMemoryAlignment > Requirements.alignment
                                             ? Backend->BufferMemoryAlignment
                                             : Requirements.alignment;

        VkMemoryAllocateInfo const AllocationInfo {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = std::clamp(Requirements.size, g_MinAllocationSize, UINT64_MAX),
                .memoryTypeIndex = ImGuiVulkanMemoryType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, Requirements.memoryTypeBits)
        };
        CheckVulkanResult(vkAllocateMemory(LogicalDevice, &AllocationInfo, nullptr, &UploadBufferMemory));
        CheckVulkanResult(vkBindBufferMemory(LogicalDevice, UploadBuffer, UploadBufferMemory, 0U));
    }

    {
        char *MappedMemory = nullptr;
        CheckVulkanResult(vkMapMemory(LogicalDevice, UploadBufferMemory, 0U, BufferSize, 0U, reinterpret_cast<void **>(&MappedMemory)));
        memcpy(MappedMemory, ImageData, BufferSize);

        VkMappedMemoryRange const MemoryRange { .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, .memory = UploadBufferMemory, .size = BufferSize };
        CheckVulkanResult(vkFlushMappedMemoryRanges(LogicalDevice, 1U, &MemoryRange));
        vkUnmapMemory(LogicalDevice, UploadBufferMemory);
    }

    {
        {
            VkImageMemoryBarrier const CopyBarrier {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = Backend->FontImage,
                    .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1U, .layerCount = 1U }
            };

            vkCmdPipelineBarrier(Backend->FontCommandBuffer,
                                 VK_PIPELINE_STAGE_HOST_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0U,
                                 0U,
                                 nullptr,
                                 0U,
                                 nullptr,
                                 1U,
                                 &CopyBarrier);
        }

        {
            VkBufferImageCopy const CopyRegion {
                    .imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1U },
                    .imageExtent = { .width = static_cast<std::uint32_t>(ImageWidth), .height = static_cast<std::uint32_t>(ImageHeight), .depth = 1U }
            };
            vkCmdCopyBufferToImage(Backend->FontCommandBuffer,
                                   UploadBuffer,
                                   Backend->FontImage,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   1U,
                                   &CopyRegion);

            VkImageMemoryBarrier const MemoryBarrier {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = Backend->FontImage,
                    .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1, }
            };

            vkCmdPipelineBarrier(Backend->FontCommandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 0U,
                                 0U,
                                 nullptr,
                                 0U,
                                 nullptr,
                                 1U,
                                 &MemoryBarrier);
        }
    }

    ImGuiIO.Fonts->SetTexID(Backend->FontDescriptorSet);

    VkSubmitInfo const SubmitInfo {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1U,
            .pCommandBuffers = &Backend->FontCommandBuffer
    };

    CheckVulkanResult(vkEndCommandBuffer(Backend->FontCommandBuffer));
    CheckVulkanResult(vkQueueSubmit(Queue, 1U, &SubmitInfo, VK_NULL_HANDLE));
    CheckVulkanResult(vkQueueWaitIdle(Queue));

    vkDestroyBuffer(LogicalDevice, UploadBuffer, nullptr);
    vkFreeMemory(LogicalDevice, UploadBufferMemory, nullptr);

    return true;
}

void RenderCore::ImGuiVulkanDestroyFontsTexture()
{
    ImGuiIO const &  ImGuiIO = ImGui::GetIO();
    ImGuiVulkanData *Backend = ImGuiVulkanGetBackendData();

    VkDevice const &LogicalDevice = GetLogicalDevice();

    if (Backend->FontDescriptorSet)
    {
        ImGuiVulkanRemoveTexture(Backend->FontDescriptorSet);
        Backend->FontDescriptorSet = VK_NULL_HANDLE;
        ImGuiIO.Fonts->SetTexID(nullptr);
    }

    if (Backend->FontView)
    {
        vkDestroyImageView(LogicalDevice, Backend->FontView, nullptr);
        Backend->FontView = VK_NULL_HANDLE;
    }

    if (Backend->FontImage)
    {
        vkDestroyImage(LogicalDevice, Backend->FontImage, nullptr);
        Backend->FontImage = VK_NULL_HANDLE;
    }

    if (Backend->FontMemory)
    {
        vkFreeMemory(LogicalDevice, Backend->FontMemory, nullptr);
        Backend->FontMemory = VK_NULL_HANDLE;
    }
}

bool RenderCore::ImGuiVulkanInit(ImGuiVulkanInitInfo const *VulkanInfo)
{
    ImGuiIO &ImGuiIO = ImGui::GetIO();

    auto *Backend                   = IM_NEW(ImGuiVulkanData)();
    ImGuiIO.BackendRendererUserData = static_cast<void *>(Backend);
    ImGuiIO.BackendRendererName     = "RenderCore_ImGui_Vulkan";
    ImGuiIO.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasViewports;

    Backend->VulkanInitInfo = *VulkanInfo;

    ImGuiVulkanCreateDeviceObjects();

    ImGuiViewport *MainViewport    = ImGui::GetMainViewport();
    MainViewport->RendererUserData = IM_NEW(ImGuiVulkanViewportData)();

    if (ImGuiIO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGuiVulkanInitPlatformInterface();
    }

    return true;
}

void RenderCore::ImGuiVulkanShutdown()
{
    ImGuiVulkanData *Backend = ImGuiVulkanGetBackendData();
    ImGuiIO &        ImGuiIO = ImGui::GetIO();

    ImGuiVulkanDestroyDeviceObjects();

    ImGuiViewport *MainViewport = ImGui::GetMainViewport();
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
    if (ImGuiVulkanData const *Backend = ImGuiVulkanGetBackendData();
        !Backend->FontDescriptorSet)
    {
        ImGuiVulkanCreateFontsTexture();
    }
}

VkDescriptorSet RenderCore::ImGuiVulkanAddTexture(VkSampler const Sampler, VkImageView const ImageView, VkImageLayout const ImageLayout)
{
    VkDescriptorSet Output;

    ImGuiVulkanData const *          Backend    = ImGuiVulkanGetBackendData();
    ImGuiVulkanInitInfo const *const&VulkanInfo = &Backend->VulkanInitInfo;

    VkDevice const &LogicalDevice = GetLogicalDevice();

    {
        VkDescriptorSetAllocateInfo const AllocationInfo {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorPool = VulkanInfo->DescriptorPool,
                .descriptorSetCount = 1U,
                .pSetLayouts = &Backend->DescriptorSetLayout
        };

        CheckVulkanResult(vkAllocateDescriptorSets(LogicalDevice, &AllocationInfo, &Output));
    }

    {
        VkDescriptorImageInfo const ImageDescriptor { .sampler = Sampler, .imageView = ImageView, .imageLayout = ImageLayout };

        VkWriteDescriptorSet const WriteDescriptor {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = Output,
                .descriptorCount = 1U,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &ImageDescriptor
        };

        vkUpdateDescriptorSets(LogicalDevice, 1U, &WriteDescriptor, 0U, nullptr);
    }

    return Output;
}

void RenderCore::ImGuiVulkanRemoveTexture(VkDescriptorSet const DescriptorSet)
{
    VkDevice const &           LogicalDevice = GetLogicalDevice();
    ImGuiVulkanData const *    Backend       = ImGuiVulkanGetBackendData();
    ImGuiVulkanInitInfo const *VulkanInfo    = &Backend->VulkanInitInfo;
    vkFreeDescriptorSets(LogicalDevice, VulkanInfo->DescriptorPool, 1, &DescriptorSet);
}

VkSurfaceFormatKHR RenderCore::ImGuiVulkanSelectSurfaceFormat(VkSurfaceKHR const    Surface,
                                                              const VkFormat *      RequestFormats,
                                                              std::int32_t const    NumRequestFormats,
                                                              VkColorSpaceKHR const RequestColorSpace)
{
    VkPhysicalDevice const &PhysicalDevice = GetPhysicalDevice();

    std::uint32_t AvailableCount = 0U;
    vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &AvailableCount, nullptr);

    ImVector<VkSurfaceFormatKHR> AvailableFormats;
    AvailableFormats.resize(static_cast<std::int32_t>(AvailableCount));
    vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &AvailableCount, AvailableFormats.Data);

    if (AvailableCount == 1U)
    {
        if (AvailableFormats[0].format == VK_FORMAT_UNDEFINED)
        {
            return VkSurfaceFormatKHR { .format = RequestFormats[0], .colorSpace = RequestColorSpace };
        }

        return AvailableFormats[0];
    }

    for (std::uint32_t RequestIt = 0U; RequestIt < NumRequestFormats; ++RequestIt)
    {
        for (std::uint32_t AvailableIt = 0U; AvailableIt < AvailableCount; ++AvailableIt)
        {
            if (AvailableFormats[AvailableIt].format == RequestFormats[RequestIt] && AvailableFormats[AvailableIt].colorSpace == RequestColorSpace)
            {
                return AvailableFormats[AvailableIt];
            }
        }
    }

    return AvailableFormats[0];
}

VkPresentModeKHR RenderCore::ImGuiVulkanSelectPresentMode(VkSurfaceKHR const      Surface,
                                                          const VkPresentModeKHR *RequestModes,
                                                          std::int32_t const      NumRequestModes)
{
    VkPhysicalDevice const &PhysicalDevice = GetPhysicalDevice();

    std::uint32_t AvailableCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &AvailableCount, nullptr);

    ImVector<VkPresentModeKHR> AvailableModes;
    AvailableModes.resize(static_cast<std::int32_t>(AvailableCount));
    vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &AvailableCount, AvailableModes.Data);

    for (std::uint32_t RequestIt = 0U; RequestIt < NumRequestModes; ++RequestIt)
    {
        for (std::uint32_t AvailableIt = 0U; AvailableIt < AvailableCount; ++AvailableIt)
        {
            if (RequestModes[RequestIt] == AvailableModes[AvailableIt])
            {
                return RequestModes[RequestIt];
            }
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

void RenderCore::ImGuiVulkanCreateOrResizeWindow(ImGuiVulkanWindow *WindowData, std::int32_t const Width, std::int32_t const Height)
{
    ImGuiVulkanCreateWindowSwapChain(WindowData, Width, Height);
    ImGuiVulkanCreateWindowCommandBuffers(WindowData);
}

void RenderCore::ImGuiVulkanDestroyWindow(ImGuiVulkanWindow *WindowData)
{
    VkDevice const &LogicalDevice = GetLogicalDevice();
    vkDeviceWaitIdle(LogicalDevice);

    for (std::uint32_t Iterator = 0U; Iterator < g_MinImageCount; Iterator++)
    {
        ImGuiVulkanDestroyFrame(&WindowData->Frames[Iterator]);
    }

    for (std::uint32_t Iterator = 0U; Iterator < WindowData->SemaphoreCount; Iterator++)
    {
        ImGuiVulkanDestroyFrameSemaphores(&WindowData->FrameSemaphores[Iterator]);
    }

    IM_FREE(WindowData->Frames);
    IM_FREE(WindowData->FrameSemaphores);
    WindowData->Frames          = nullptr;
    WindowData->FrameSemaphores = nullptr;

    vkDestroyPipeline(LogicalDevice, WindowData->Pipeline, nullptr);
    vkDestroySwapchainKHR(LogicalDevice, WindowData->Swapchain, nullptr);
    vkDestroySurfaceKHR(GetInstance(), WindowData->Surface, nullptr);

    *WindowData = ImGuiVulkanWindow();
}
