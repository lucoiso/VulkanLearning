// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <string_view>
#include <Volk/volk.h>

module RenderCore.Types.Texture;

import RenderCore.Runtime.Memory;
import RenderCore.Runtime.Scene;

using namespace RenderCore;

Texture::Texture(std::uint32_t const ID, std::string_view const Path)
    : Resource(ID, Path)
{
}

Texture::Texture(std::uint32_t const ID, std::string_view const Path, std::string_view const Name)
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
