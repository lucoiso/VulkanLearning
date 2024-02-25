// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <array>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <vector>
#include <Volk/volk.h>
#include <boost/log/trivial.hpp>

module RenderCore.Management.ImGuiManagement;

import RenderCore.Renderer;
import RenderCore.Management.DeviceManagement;
import RenderCore.Management.PipelineManagement;
import RenderCore.Management.CommandsManagement;
import RenderCore.Utils.Constants;
import RenderCore.Utils.Helpers;
import RenderCore.Types.Camera;
import RuntimeInfo.Manager;

using namespace RenderCore;

VkDescriptorPool g_ImGuiDescriptorPool{VK_NULL_HANDLE};

void RenderCore::InitializeImGuiContext(GLFWwindow* const Window, SurfaceProperties const& SurfaceProperties)
{
    auto const _ { RuntimeInfo::Manager::Get().PushCallstackWithCounter() };
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating ImGui Context";

    IMGUI_CHECKVERSION();

    ImGui_ImplVulkan_LoadFunctions([](const char* FunctionName, void*) {
        return vkGetInstanceProcAddr(volkGetLoadedInstance(), FunctionName);
    });

    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& ImIO = ImGui::GetIO();
    (void)ImIO;

    if (ImIO.BackendPlatformUserData)
    {
        return;
    }

    ImIO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable | ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForVulkan(Window, false);
    ImGui_ImplGlfw_SetCallbacksChainForAllWindows(true);
    ImGui_ImplGlfw_InstallCallbacks(Window);

    constexpr std::uint32_t DescriptorCount = 100U;

    constexpr std::array DescriptorPoolSizes{
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLER, DescriptorCount},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, DescriptorCount},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, DescriptorCount},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, DescriptorCount},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, DescriptorCount},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, DescriptorCount},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, DescriptorCount},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, DescriptorCount},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, DescriptorCount},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, DescriptorCount},
            VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, DescriptorCount}};

    VkDescriptorPoolCreateInfo const DescriptorPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = static_cast<std::uint32_t>(std::size(DescriptorPoolSizes)),
            .poolSizeCount = std::size(DescriptorPoolSizes),
            .pPoolSizes = std::data(DescriptorPoolSizes)};

    CheckVulkanResult(vkCreateDescriptorPool(GetLogicalDevice(), &DescriptorPoolCreateInfo, nullptr, &g_ImGuiDescriptorPool));

    ImGui_ImplVulkan_InitInfo ImGuiVulkanInitInfo{
            .Instance = volkGetLoadedInstance(),
            .PhysicalDevice = GetPhysicalDevice(),
            .Device = GetLogicalDevice(),
            .QueueFamily = GetGraphicsQueue().first,
            .Queue = GetGraphicsQueue().second,
            .PipelineCache = VK_NULL_HANDLE,
            .DescriptorPool = g_ImGuiDescriptorPool,
            .Subpass = 0U,
            .MinImageCount = g_MinImageCount,
            .ImageCount = g_MinImageCount,
            .MSAASamples = g_MSAASamples,
            .UseDynamicRendering = true,
            .ColorAttachmentFormat = SurfaceProperties.Format.format,
            .Allocator = VK_NULL_HANDLE,
            .CheckVkResultFn = [](VkResult Result) {
                CheckVulkanResult(Result);
            }};

    ImGui_ImplVulkan_Init(&ImGuiVulkanInitInfo, VK_NULL_HANDLE);
    ImGui_ImplVulkan_CreateFontsTexture();
}

void RenderCore::ReleaseImGuiResources()
{
    auto const _ { RuntimeInfo::Manager::Get().PushCallstackWithCounter() };

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (g_ImGuiDescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(volkGetLoadedDevice(), g_ImGuiDescriptorPool, nullptr);
        g_ImGuiDescriptorPool = VK_NULL_HANDLE;
    }
}

void RenderCore::DrawImGuiFrame(std::function<void()>&& PreDraw, std::function<void()>&& Draw, std::function<void()>&& PostDraw)
{
    if (!ImGui::GetCurrentContext() || !IsImGuiInitialized())
    {
        return;
    }

    PreDraw(); {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame(); {
            Draw();
        }
        ImGui::Render();

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
    PostDraw();
}

bool RenderCore::IsImGuiInitialized()
{
    return g_ImGuiDescriptorPool != VK_NULL_HANDLE;
}
