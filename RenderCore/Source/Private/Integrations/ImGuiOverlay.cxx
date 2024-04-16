// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
#include <array>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <vector>
#include <boost/log/trivial.hpp>
#include <Volk/volk.h>
#endif

module RenderCore.Integrations.ImGuiOverlay;

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
import RenderCore.Renderer;
import RenderCore.Runtime.Device;
import RenderCore.Runtime.Pipeline;
import RenderCore.Runtime.Command;
import RenderCore.Runtime.SwapChain;
import RenderCore.Utils.Constants;
import RenderCore.Utils.Helpers;
import RenderCore.Types.Camera;
import RenderCore.Types.Allocation;

using namespace RenderCore;

VkDescriptorPool g_ImGuiDescriptorPool { VK_NULL_HANDLE };

void RenderCore::InitializeImGuiContext(GLFWwindow *const Window, SurfaceProperties const &SurfaceProperties)
{
    IMGUI_CHECKVERSION();

    ImGui_ImplVulkan_LoadFunctions([](const char *FunctionName, void *)
    {
        return vkGetInstanceProcAddr(volkGetLoadedInstance(), FunctionName);
    });

    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO &ImIO = ImGui::GetIO();
    (void)ImIO;

    if (ImIO.BackendPlatformUserData)
    {
        return;
    }

    ImIO.ConfigFlags |= /*ImGuiConfigFlags_ViewportsEnable |*/ ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForVulkan(Window, false);
    ImGui_ImplGlfw_SetCallbacksChainForAllWindows(true);
    ImGui_ImplGlfw_InstallCallbacks(Window);

    constexpr std::uint32_t DescriptorCount = 100U;

    constexpr std::array DescriptorPoolSizes {
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_SAMPLER, DescriptorCount },
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, DescriptorCount },
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, DescriptorCount },
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, DescriptorCount },
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, DescriptorCount },
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, DescriptorCount },
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, DescriptorCount },
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, DescriptorCount },
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, DescriptorCount },
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, DescriptorCount },
            VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, DescriptorCount }
    };

    VkDescriptorPoolCreateInfo const DescriptorPoolCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = static_cast<std::uint32_t>(std::size(DescriptorPoolSizes)),
            .poolSizeCount = std::size(DescriptorPoolSizes),
            .pPoolSizes = std::data(DescriptorPoolSizes)
    };

    CheckVulkanResult(vkCreateDescriptorPool(GetLogicalDevice(), &DescriptorPoolCreateInfo, nullptr, &g_ImGuiDescriptorPool));

    std::vector const ColorAttachmentFormat { SurfaceProperties.Format.format };

    auto const &[QueueFamilyIndex, Queue] = GetGraphicsQueue();

    ImGui_ImplVulkan_InitInfo ImGuiVulkanInitInfo {
            .Instance = volkGetLoadedInstance(),
            .PhysicalDevice = GetPhysicalDevice(),
            .Device = GetLogicalDevice(),
            .QueueFamily = QueueFamilyIndex,
            .Queue = Queue,
            .DescriptorPool = g_ImGuiDescriptorPool,
            .RenderPass = VK_NULL_HANDLE,
            .MinImageCount = g_MinImageCount,
            .ImageCount = g_MinImageCount,
            .MSAASamples = g_MSAASamples,
            .PipelineCache = VK_NULL_HANDLE,
            .Subpass = 0U,
            .UseDynamicRendering = true,
            .PipelineRenderingCreateInfo = {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                    .colorAttachmentCount = static_cast<std::uint32_t>(std::size(ColorAttachmentFormat)),
                    .pColorAttachmentFormats = std::data(ColorAttachmentFormat),
                    .depthAttachmentFormat = SurfaceProperties.DepthFormat,
                    .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
            },
            .Allocator = VK_NULL_HANDLE,
            .CheckVkResultFn = [](VkResult Result)
            {
                CheckVulkanResult(Result);
            }
    };

    ImGui_ImplVulkan_Init(&ImGuiVulkanInitInfo);
    ImGui_ImplVulkan_CreateFontsTexture();
}

void RenderCore::ReleaseImGuiResources()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (g_ImGuiDescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(volkGetLoadedDevice(), g_ImGuiDescriptorPool, nullptr);
        g_ImGuiDescriptorPool = VK_NULL_HANDLE;
    }
}

void RenderCore::DrawImGuiFrame(Control *Control)
{
    if (!ImGui::GetCurrentContext() || !IsImGuiInitialized())
    {
        return;
    }

    Control->PreUpdate();
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            Control->Update();
        }
        ImGui::Render();

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
    Control->PostUpdate();
}

bool RenderCore::IsImGuiInitialized()
{
    return g_ImGuiDescriptorPool != VK_NULL_HANDLE;
}

void RenderCore::RecordImGuiCommandBuffer(VkCommandBuffer const &CommandBuffer,
                                          std::uint32_t const    ImageIndex,
                                          VkImageLayout const    SwapChainMidLayout)
{
    if (IsImGuiInitialized())
    {
        if (ImDrawData *const ImGuiDrawData = ImGui::GetDrawData())
        {
            ImageAllocation const &SwapchainAllocation = GetSwapChainImages().at(ImageIndex);

            VkRenderingAttachmentInfo const ColorAttachmentInfo {
                    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                    .imageView = SwapchainAllocation.View,
                    .imageLayout = SwapChainMidLayout,
                    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue = g_ClearValues.at(0U)
            };

            VkRenderingInfo const RenderingInfo {
                    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                    .renderArea = { .offset = { 0, 0 }, .extent = SwapchainAllocation.Extent },
                    .layerCount = 1U,
                    .colorAttachmentCount = 1U,
                    .pColorAttachments = &ColorAttachmentInfo
            };

            vkCmdBeginRendering(CommandBuffer, &RenderingInfo);
            ImGui_ImplVulkan_RenderDrawData(ImGuiDrawData, CommandBuffer);
            vkCmdEndRendering(CommandBuffer);
        }
    }
}
#endif
