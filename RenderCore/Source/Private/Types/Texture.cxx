// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.Types.Texture;

import RenderCore.Renderer;
import RenderCore.Runtime.Memory;
import RenderCore.Runtime.Scene;
import RenderCore.Utils.Constants;
import RenderCore.Integrations.ImGuiVulkanBackend;

using namespace RenderCore;

Texture::Texture(std::uint32_t const ID, strzilla::string_view const Path)
    : Resource(ID, Path)
{
}

Texture::Texture(std::uint32_t const ID, strzilla::string_view const Path, strzilla::string_view const Name)
    : Resource(ID, Path, Name)
{
}

std::vector<TextureType> Texture::GetTypes() const
{
    return m_Types;
}

void Texture::SetTypes(std::vector<TextureType> const &Types)
{
    m_Types = Types;
}

void Texture::AppendType(TextureType const Type)
{
    if (std::ranges::find(m_Types, Type) != std::end(m_Types))
    {
        return;
    }

    m_Types.push_back(Type);
}

void Texture::SetupTexture()
{
    m_ImageDescriptor = GetAllocationImageDescriptor(GetID() == UINT32_MAX ? 0U : GetBufferIndex());
}

VkDescriptorImageInfo Texture::GetImageDescriptor() const
{
    return m_ImageDescriptor;
}

VkDescriptorSet const & Texture::GetDescriptorSet() const
{
    return m_DescriptorSet;
}

void Texture::SetRenderAsImage(bool const Value)
{
    if (m_RenderAsImage == Value)
    {
        return;
    }

    m_RenderAsImage = Value;
    UpdateImageRendering();
}

void Texture::UpdateImageRendering()
{
    if (!Renderer::IsImGuiInitialized())
    {
        return;
    }

    if (m_DescriptorSet != VK_NULL_HANDLE)
    {
        ImGuiVulkanRemoveTexture(m_DescriptorSet);
    }

    if (m_RenderAsImage)
    {
        VkSampler const Sampler { Renderer::GetSampler() };
        VkImageView const View = GetImageDescriptor().imageView;

        if (Sampler != VK_NULL_HANDLE && View != VK_NULL_HANDLE)
        {
            m_DescriptorSet = ImGuiVulkanAddTexture(Sampler, View, g_ReadLayout);
        }
    }
}
