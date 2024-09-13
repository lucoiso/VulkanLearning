// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.Types.Texture;

import RenderCore.Runtime.Memory;

using namespace RenderCore;

Texture::Texture(std::uint32_t const ID, strzilla::string_view const Path)
    : Resource(ID, Path)
{
}

Texture::Texture(std::uint32_t const ID, strzilla::string_view const Path, strzilla::string_view const Name)
    : Resource(ID, Path, Name)
{
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
