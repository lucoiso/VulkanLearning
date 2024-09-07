// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.UserInterface.Viewport;

import RenderCore.Renderer;
import RenderCore.Types.Camera;
import RenderCore.Types.Object;
import RenderCore.Utils.Constants;
import RenderCore.Integrations.ImGuiVulkanBackend;
import RenderCore.Integrations.GLFWCallbacks;

using namespace RenderCore;

Viewport::Viewport(Control *const Parent)
    : Control(Parent)
{
    Renderer::SetRenderOffscreen(true);
    TakeCameraControl(true);
}

Viewport::~Viewport()
{
    EASY_FUNCTION(profiler::colors::Yellow);

    if (!std::empty(m_ViewportDescriptorSets))
    {
        for (auto const &DescriptorSetIter : m_ViewportDescriptorSets)
        {
            if (DescriptorSetIter != VK_NULL_HANDLE)
            {
                ImGuiVulkanRemoveTexture(DescriptorSetIter);
            }
        }
        m_ViewportDescriptorSets.clear();
    }
}

void Viewport::TakeCameraControl(bool const Value) const
{
    SetViewportControlsCamera(Value);
}

bool Viewport::IsControllingCamera() const
{
    return ViewportControlsCamera();
}

void Viewport::Refresh()
{
    EASY_FUNCTION(profiler::colors::Yellow);

    if (!Renderer::IsImGuiInitialized())
    {
        return;
    }

    if (!std::empty(m_ViewportDescriptorSets))
    {
        for (auto const &DescriptorSetIter : m_ViewportDescriptorSets)
        {
            if (DescriptorSetIter != VK_NULL_HANDLE)
            {
                ImGuiVulkanRemoveTexture(DescriptorSetIter);
            }
        }
        m_ViewportDescriptorSets.clear();
    }

    VkSampler const Sampler { Renderer::GetSampler() };

    if (std::vector const ImageViews { Renderer::GetOffscreenImages() };
        Sampler != VK_NULL_HANDLE && !std::empty(ImageViews))
    {
        for (auto const &ImageViewIter : ImageViews)
        {
            m_ViewportDescriptorSets.push_back(ImGuiVulkanAddTexture(Sampler, ImageViewIter, g_ReadLayout));
        }
    }
}

void Viewport::PrePaint()
{
    EASY_FUNCTION(profiler::colors::Yellow);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4 { 0.F, 0.F, 0.F, 1.F });

    m_Open = ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoMove) && ImGui::IsItemVisible();
    ImGui::PopStyleColor();

    SetViewportHovering(ImGui::IsWindowHovered());
}

void Viewport::Paint()
{
    EASY_FUNCTION(profiler::colors::Yellow);

    if (m_Open && !std::empty(m_ViewportDescriptorSets))
    {
        if (std::optional<std::int32_t> const &ImageIndex = Renderer::GetImageIndex();
            ImageIndex.has_value())
        {
            ImVec2 const ViewportSize { ImGui::GetContentRegionAvail() };
            ImGui::Image(m_ViewportDescriptorSets.at(ImageIndex.value()), ImVec2 { ViewportSize.x, ViewportSize.y });
        }
    }
}

void Viewport::PostPaint()
{
    EASY_FUNCTION(profiler::colors::Yellow);

    ImGui::End();
}
