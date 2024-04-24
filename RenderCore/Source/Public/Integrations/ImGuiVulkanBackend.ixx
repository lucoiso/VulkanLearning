// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

// Adapted from: https://github.com/ocornut/imgui/blob/docking/backends/imgui_impl_vulkan.cpp

module;

#include <cstdint>
#include <imgui.h>
#include <Volk/volk.h>
#include "RenderCoreModule.hpp"

export module RenderCore.Integrations.ImGuiVulkanBackend;

export namespace RenderCore
{
    struct ImGuiVulkanInitInfo
    {
        VkDescriptorPool              DescriptorPool;
        VkPipelineRenderingCreateInfo PipelineRenderingCreateInfo;
    };

    struct ImGuiVulkanH_Frame
    {
        VkCommandPool   CommandPool;
        VkCommandBuffer CommandBuffer;
        VkFence         Fence;
        VkImage         Backbuffer;
        VkImageView     BackbufferView;
    };

    struct ImGuiVulkanH_FrameSemaphores
    {
        VkSemaphore ImageAcquiredSemaphore;
        VkSemaphore RenderCompleteSemaphore;
    };

    struct ImGuiVulkanH_Window
    {
        std::int32_t                  Width {};
        std::int32_t                  Height {};
        VkSwapchainKHR                Swapchain {};
        VkSurfaceKHR                  Surface {};
        VkSurfaceFormatKHR            SurfaceFormat {};
        VkPresentModeKHR              PresentMode;
        VkPipeline                    Pipeline {};
        bool                          ClearEnable;
        VkClearValue                  ClearValue {};
        std::uint32_t                 FrameIndex {};
        std::uint32_t                 ImageCount {};
        std::uint32_t                 SemaphoreCount {};
        std::uint32_t                 SemaphoreIndex {};
        ImGuiVulkanH_Frame *          Frames {};
        ImGuiVulkanH_FrameSemaphores *FrameSemaphores {};

        ImGuiVulkanH_Window()
        {
            memset(this, 0, sizeof(*this));
            PresentMode = static_cast<VkPresentModeKHR>(~0);
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
        ImGuiVulkanH_Window            Window {};
        ImGuiVulkanWindowRenderBuffers RenderBuffers {};

        ImGuiVulkanViewportData()
        {
            WindowOwned = false;
            memset(&RenderBuffers, 0, sizeof(RenderBuffers));
        }

        ~ImGuiVulkanViewportData() = default;
    };

    struct ImGuiVulkanData
    {
        ImGuiVulkanInitInfo   VulkanInitInfo {};
        VkDeviceSize          BufferMemoryAlignment;
        VkPipelineCreateFlags PipelineCreateFlags {};
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
            memset(this, 0, sizeof(*this));
            BufferMemoryAlignment = 256;
        }
    };

    bool ImGuiVulkanInit(ImGuiVulkanInitInfo const *);
    void ImGuiVulkanShutdown();
    void ImGuiVulkanNewFrame();
    void ImGuiVulkanRenderDrawData(ImDrawData *, VkCommandBuffer);
    bool ImGuiVulkanCreateFontsTexture();
    void ImGuiVulkanDestroyFontsTexture();

    bool ImGuiVulkanCreateDeviceObjects();
    void ImGuiVulkanDestroyDeviceObjects();

    void ImGuiVulkanDestroyFrameRenderBuffers(ImGuiVulkanFrameRenderBuffers *);
    void ImGuiVulkanDestroyWindowRenderBuffers(ImGuiVulkanWindowRenderBuffers *);

    void ImGuiVulkanH_DestroyFrame(ImGuiVulkanH_Frame *);
    void ImGuiVulkanH_DestroyFrameSemaphores(ImGuiVulkanH_FrameSemaphores *);
    void ImGuiVulkanH_DestroyAllViewportsRenderBuffers();

    void ImGuiVulkanH_CreateWindowSwapChain(ImGuiVulkanH_Window *, std::int32_t, std::int32_t);

    void ImGuiVulkanH_CreateWindowCommandBuffers(ImGuiVulkanH_Window const *);

    void ImGuiVulkanInitPlatformInterface();
    void ImGuiVulkanShutdownPlatformInterface();

    RENDERCOREMODULE_API VkDescriptorSet ImGuiVulkanAddTexture(VkSampler, VkImageView, VkImageLayout);
    RENDERCOREMODULE_API void            ImGuiVulkanRemoveTexture(VkDescriptorSet);

    void ImGuiVulkanH_CreateOrResizeWindow(ImGuiVulkanH_Window *, std::int32_t, std::int32_t);

    void ImGuiVulkanH_DestroyWindow(ImGuiVulkanH_Window *);

    VkSurfaceFormatKHR ImGuiVulkanH_SelectSurfaceFormat(VkSurfaceKHR, const VkFormat *, std::int32_t, VkColorSpaceKHR);

    VkPresentModeKHR ImGuiVulkanH_SelectPresentMode(VkSurfaceKHR, const VkPresentModeKHR *, std::int32_t);

    std::int32_t ImGuiVulkanH_GetMinImageCountFromPresentMode(VkPresentModeKHR);
}
