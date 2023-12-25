// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <array>
#include <boost/log/trivial.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <vector>
#include <volk.h>

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

VkRenderPass g_ImGuiRenderPass {VK_NULL_HANDLE};
std::vector<VkFramebuffer> g_ImGuiFrameBuffers {};
VkDescriptorPool g_ImGuiDescriptorPool {VK_NULL_HANDLE};

void RenderCore::InitializeImGuiContext(GLFWwindow* const Window)
{
    if (g_ImGuiRenderPass == VK_NULL_HANDLE)
    {
        return;
    }

    Timer::ScopedTimer const ScopedExecutionTimer(__func__);
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating ImGui Context";

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
            .maxSets       = static_cast<std::uint32_t>(std::size(DescriptorPoolSizes)),
            .poolSizeCount = std::size(DescriptorPoolSizes),
            .pPoolSizes    = std::data(DescriptorPoolSizes)};

    CheckVulkanResult(vkCreateDescriptorPool(GetLogicalDevice(), &DescriptorPoolCreateInfo, nullptr, &g_ImGuiDescriptorPool));

    ImGui_ImplVulkan_InitInfo ImGuiVulkanInitInfo {
            .Instance        = volkGetLoadedInstance(),
            .PhysicalDevice  = GetPhysicalDevice(),
            .Device          = GetLogicalDevice(),
            .QueueFamily     = GetGraphicsQueue().first,
            .Queue           = GetGraphicsQueue().second,
            .PipelineCache   = VK_NULL_HANDLE,
            .DescriptorPool  = g_ImGuiDescriptorPool,
            .Subpass         = 0U,
            .MinImageCount   = g_MinImageCount,
            .ImageCount      = g_MinImageCount,
            .MSAASamples     = g_MSAASamples,
            .Allocator       = VK_NULL_HANDLE,
            .CheckVkResultFn = [](VkResult Result) {
                CheckVulkanResult(Result);
            }};

    ImGui_ImplVulkan_Init(&ImGuiVulkanInitInfo, g_ImGuiRenderPass);

    std::vector<VkCommandBuffer> CommandBuffer {VK_NULL_HANDLE};
    VkCommandPool CommandPool = VK_NULL_HANDLE;
    InitializeSingleCommandQueue(CommandPool, CommandBuffer, GetGraphicsQueue().first);
    {
        ImGui_ImplVulkan_CreateFontsTexture(CommandBuffer.back());
    }
    FinishSingleCommandQueue(GetGraphicsQueue().second, CommandPool, CommandBuffer);

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void RenderCore::CreateImGuiRenderPass(SurfaceProperties const& SurfaceProperties)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating ImGui Render Pass";

    VkAttachmentDescription const Attachment {
            .format         = SurfaceProperties.Format.format,
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};

    VkAttachmentReference const ColorAttachment {
            .attachment = 0U,
            .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription const Subpass {
            .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1U,
            .pColorAttachments    = &ColorAttachment};

    VkSubpassDependency const Dependency {
            .srcSubpass    = VK_SUBPASS_EXTERNAL,
            .dstSubpass    = 0U,
            .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0U,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};

    VkRenderPassCreateInfo const RenderPassCreateInfo {
            .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1U,
            .pAttachments    = &Attachment,
            .subpassCount    = 1U,
            .pSubpasses      = &Subpass,
            .dependencyCount = 1U,
            .pDependencies   = &Dependency};

    CheckVulkanResult(vkCreateRenderPass(volkGetLoadedDevice(), &RenderPassCreateInfo, nullptr, &g_ImGuiRenderPass));
}

void RenderCore::DestroyImGuiRenderPass()
{
    if (g_ImGuiRenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(volkGetLoadedDevice(), g_ImGuiRenderPass, nullptr);
        g_ImGuiRenderPass = VK_NULL_HANDLE;
    }
}

void RenderCore::CreateImGuiFrameBuffers(BufferManager const& BufferManager)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating ImGui Frame Buffers";

    DestroyImGuiFrameBuffers();

    g_ImGuiFrameBuffers = BufferManager.CreateFrameBuffers(g_ImGuiRenderPass, BufferManager.GetSwapChainImages(), false);
}

void RenderCore::DestroyImGuiFrameBuffers()
{
    if (!std::empty(g_ImGuiFrameBuffers))
    {
        for (VkFramebuffer const& FrameBufferIter: g_ImGuiFrameBuffers)
        {
            vkDestroyFramebuffer(volkGetLoadedDevice(), FrameBufferIter, nullptr);
        }
        g_ImGuiFrameBuffers.clear();
    }
}

void RenderCore::ReleaseImGuiResources()
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    DestroyImGuiRenderPass();
    DestroyImGuiFrameBuffers();

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

    PreDraw();
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
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
    return g_ImGuiDescriptorPool != VK_NULL_HANDLE && g_ImGuiRenderPass != VK_NULL_HANDLE && !std::empty(g_ImGuiFrameBuffers);
}

VkRenderPass const& RenderCore::GetImGuiRenderPass()
{
    return g_ImGuiRenderPass;
}

std::vector<VkFramebuffer> const& RenderCore::GetImGuiFrameBuffers()
{
    return g_ImGuiFrameBuffers;
}