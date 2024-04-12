// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <Volk/volk.h>
#include <string_view>

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
    if (GetID() == UINT32_MAX)
    {
        m_ImageDescriptor = VkDescriptorImageInfo {
                .sampler = GetSampler(),
                .imageView = GetEmptyImage().View,
                .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR
        };
    }
    else
    {
        m_ImageDescriptor = GetAllocationImageDescriptor(GetBufferIndex());
    }
}

std::vector<VkWriteDescriptorSet> Texture::GetWriteDescriptorSet() const
{
    std::vector<VkWriteDescriptorSet> Output;
    Output.reserve(std::size(m_Types));

    for (TextureType const TypeIter : m_Types)
    {
        Output.push_back(VkWriteDescriptorSet {
                                 .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                 .dstSet = VK_NULL_HANDLE,
                                 .dstBinding = 2U + static_cast<std::uint32_t>(TypeIter),
                                 .dstArrayElement = 0U,
                                 .descriptorCount = 1U,
                                 .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                 .pImageInfo = &m_ImageDescriptor,
                         });
    }

    return Output;
}
