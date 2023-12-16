// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <volk.h>
#include <array>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

module RenderCore.Management.ImGuiManagement;

import RenderCore.Renderer;
import RenderCore.Management.DeviceManagement;
import RenderCore.Management.PipelineManagement;
import RenderCore.Management.CommandsManagement;
import RenderCore.Utils.Constants;
import RenderCore.Utils.Helpers;
import RenderCore.Types.Camera;
import Timer.ExecutionCounter;

using namespace RenderCore;

VkDescriptorPool g_ImGuiDescriptorPool {VK_NULL_HANDLE};

void RenderCore::InitializeImGui(GLFWwindow* const Window, PipelineManager& PipelineManager)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    IMGUI_CHECKVERSION();

    ImGui_ImplVulkan_LoadFunctions([](const char* FunctionName, void*) {
        return vkGetInstanceProcAddr(volkGetLoadedInstance(), FunctionName);
    });

    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    if (ImGui::GetIO().BackendPlatformUserData)
    {
        return;
    }

    PipelineManager.SetIsBoundToImGui(true);
    ImGui_ImplGlfw_InitForVulkan(Window, true);

    constexpr std::uint32_t DescriptorCount = 100U;

    constexpr std::array DescriptorPoolSizes {
            VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_SAMPLER, DescriptorCount},
            VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, DescriptorCount},
            VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, DescriptorCount},
            VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, DescriptorCount},
            VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, DescriptorCount},
            VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, DescriptorCount},
            VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, DescriptorCount},
            VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, DescriptorCount},
            VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, DescriptorCount},
            VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, DescriptorCount},
            VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, DescriptorCount}};

    VkDescriptorPoolCreateInfo const DescriptorPoolCreateInfo {
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets       = 1000U,
            .poolSizeCount = std::size(DescriptorPoolSizes),
            .pPoolSizes    = std::data(DescriptorPoolSizes)};

    CheckVulkanResult(vkCreateDescriptorPool(GetLogicalDevice(), &DescriptorPoolCreateInfo, nullptr, &g_ImGuiDescriptorPool));

    ImGui_ImplVulkan_InitInfo ImGuiVulkanInitInfo {
            .Instance        = volkGetLoadedInstance(),
            .PhysicalDevice  = GetPhysicalDevice(),
            .Device          = GetLogicalDevice(),
            .QueueFamily     = GetGraphicsQueue().first,
            .Queue           = GetGraphicsQueue().second,
            .PipelineCache   = PipelineManager.GetPipelineCache(),
            .DescriptorPool  = g_ImGuiDescriptorPool,
            .MinImageCount   = g_MinImageCount,
            .ImageCount      = g_MinImageCount,
            .MSAASamples     = g_MSAASamples,
            .Allocator       = VK_NULL_HANDLE,
            .CheckVkResultFn = [](VkResult Result) {
                CheckVulkanResult(Result);
            }};

    ImGui_ImplVulkan_Init(&ImGuiVulkanInitInfo, PipelineManager.GetRenderPass());

    std::vector<VkCommandBuffer> CommandBuffer {VK_NULL_HANDLE};
    VkCommandPool CommandPool = VK_NULL_HANDLE;
    InitializeSingleCommandQueue(CommandPool, CommandBuffer, GetGraphicsQueue().first);
    {
        ImGui_ImplVulkan_CreateFontsTexture(CommandBuffer.back());
    }
    FinishSingleCommandQueue(GetGraphicsQueue().second, CommandPool, CommandBuffer);

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void RenderCore::ReleaseImGuiResources()
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    if (g_ImGuiDescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(volkGetLoadedDevice(), g_ImGuiDescriptorPool, nullptr);
        g_ImGuiDescriptorPool = VK_NULL_HANDLE;
    }

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void RenderCore::DrawImGuiFrame(std::function<void()>&& OverlayDrawFunction)
{
    if (!ImGui::GetCurrentContext())
    {
        return;
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    OverlayDrawFunction();

    ImGui::Render();
}