// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

// Adapted from: https://github.com/ocornut/imgui/blob/docking/backends/imgui_impl_vulkan.cpp

module;

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
#include <cstdint>
#include <imgui.h>
#include <Volk/volk.h>
#include "RenderCoreModule.hpp"
#endif

export module RenderCore.Integrations.ImGuiVulkanBackend;

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
import RenderCore.Types.Allocation;

struct ImGuiVulkanWindowRenderBuffers;
struct ImGuiVulkanFrame;
struct ImGuiVulkanFrameSemaphores;
struct ImGuiVulkanWindow;
#endif

namespace RenderCore
{
    #ifdef VULKAN_RENDERER_ENABLE_IMGUI
    export struct ImGuiVulkanInitInfo
    {
        VkDescriptorPool              DescriptorPool;
        VkPipelineRenderingCreateInfo PipelineRenderingCreateInfo;
    };

    export bool ImGuiVulkanInit(ImGuiVulkanInitInfo const *);
    export void ImGuiVulkanShutdown();
    export void ImGuiVulkanNewFrame();
    export void ImGuiVulkanRenderDrawData(ImDrawData *, VkCommandBuffer);
    export bool ImGuiVulkanCreateFontsTexture();
    export void ImGuiVulkanDestroyFontsTexture();

    export RENDERCOREMODULE_API VkDescriptorSet ImGuiVulkanAddTexture(VkSampler, VkImageView, VkImageLayout);
    export RENDERCOREMODULE_API void            ImGuiVulkanRemoveTexture(VkDescriptorSet);

    bool ImGuiVulkanCreateDeviceObjects();
    void ImGuiVulkanDestroyDeviceObjects();

    void ImGuiVulkanDestroyFrameRenderBuffers(BufferAllocation &);
    void ImGuiVulkanDestroyWindowRenderBuffers(ImGuiVulkanWindowRenderBuffers &);

    void ImGuiVulkanDestroyFrame(ImGuiVulkanFrame &);
    void ImGuiVulkanDestroyFrameSemaphores(ImGuiVulkanFrameSemaphores *);
    void ImGuiVulkanDestroyAllViewportsRenderBuffers();

    void ImGuiVulkanCreateWindowSwapChain(ImGuiVulkanWindow &, std::int32_t, std::int32_t);

    void ImGuiVulkanCreateWindowCommandBuffers(ImGuiVulkanWindow &);

    void ImGuiVulkanInitPlatformInterface();
    void ImGuiVulkanShutdownPlatformInterface();

    void ImGuiVulkanCreateOrResizeWindow(ImGuiVulkanWindow &, std::int32_t, std::int32_t);

    void ImGuiVulkanDestroyWindow(ImGuiVulkanWindow &);

    VkSurfaceFormatKHR ImGuiVulkanSelectSurfaceFormat(VkSurfaceKHR, const VkFormat *, std::int32_t, VkColorSpaceKHR);

    VkPresentModeKHR ImGuiVulkanSelectPresentMode(VkSurfaceKHR, const VkPresentModeKHR *, std::int32_t);
    #endif
}
