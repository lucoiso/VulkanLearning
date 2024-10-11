// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.Types.Object;

import RenderCore.Renderer;

using namespace RenderCore;

Object::Object(std::uint32_t const ID, strzilla::string_view const Path)
    : Resource(ID, Path)
{
}

Object::Object(std::uint32_t const ID, strzilla::string_view const Path, strzilla::string_view const Name)
    : Resource(ID, Path, Name)
{
}

void Object::Destroy()
{
    Resource::Destroy();
    Renderer::RequestUnloadObjects({ GetID() });
}

void Object::DrawObject(VkCommandBuffer const& CommandBuffer, VkPipelineLayout const& PipelineLayout, std::uint32_t const ObjectIndex) const
{
    if (m_Mesh)
    {
        m_Mesh->DrawObject(CommandBuffer, PipelineLayout, ObjectIndex);
    }
}
