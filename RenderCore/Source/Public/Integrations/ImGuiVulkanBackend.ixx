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
struct ImGuiVulkanFrameRenderBuffers;
struct ImGuiVulkanWindowRenderBuffers;
struct ImGuiVulkanFrame;
struct ImGuiVulkanFrameSemaphores;
struct ImGuiVulkanWindow;
#endif

export namespace RenderCore
{
    #ifdef VULKAN_RENDERER_ENABLE_IMGUI
    struct ImGuiVulkanInitInfo
    {
        VkDescriptorPool              DescriptorPool;
        VkPipelineRenderingCreateInfo PipelineRenderingCreateInfo;
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

    void ImGuiVulkanDestroyFrame(ImGuiVulkanFrame *);
    void ImGuiVulkanDestroyFrameSemaphores(ImGuiVulkanFrameSemaphores *);
    void ImGuiVulkanDestroyAllViewportsRenderBuffers();

    void ImGuiVulkanCreateWindowSwapChain(ImGuiVulkanWindow *, std::int32_t, std::int32_t);

    void ImGuiVulkanCreateWindowCommandBuffers(ImGuiVulkanWindow const *);

    void ImGuiVulkanInitPlatformInterface();
    void ImGuiVulkanShutdownPlatformInterface();

    RENDERCOREMODULE_API VkDescriptorSet ImGuiVulkanAddTexture(VkSampler, VkImageView, VkImageLayout);
    RENDERCOREMODULE_API void            ImGuiVulkanRemoveTexture(VkDescriptorSet);

    void ImGuiVulkanCreateOrResizeWindow(ImGuiVulkanWindow *, std::int32_t, std::int32_t);

    void ImGuiVulkanDestroyWindow(ImGuiVulkanWindow *);

    VkSurfaceFormatKHR ImGuiVulkanSelectSurfaceFormat(VkSurfaceKHR, const VkFormat *, std::int32_t, VkColorSpaceKHR);

    VkPresentModeKHR ImGuiVulkanSelectPresentMode(VkSurfaceKHR, const VkPresentModeKHR *, std::int32_t);

    std::int32_t ImGuiVulkanGetMinImageCountFromPresentMode(VkPresentModeKHR);
    #endif
}
