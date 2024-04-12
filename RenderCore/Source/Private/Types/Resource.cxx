// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <string_view>

module RenderCore.Types.Resource;

using namespace RenderCore;

Resource::Resource(std::uint32_t const ID, std::string_view const Path)
    : m_ID(ID)
  , m_Path(Path)
  , m_Name(m_Path.substr(m_Path.find_last_of('\\') + 1, m_Path.find_last_of('.') - m_Path.find_last_of('\\') - 1))
{
}

Resource::Resource(std::uint32_t const ID, std::string_view const Path, std::string_view const Name)
    : m_ID(ID)
  , m_Path(Path)
  , m_Name(Name)
{
}

std::uint32_t Resource::GetID() const
{
    return m_ID;
}

std::string const &Resource::GetPath() const
{
    return m_Path;
}

std::string const &Resource::GetName() const
{
    return m_Name;
}

std::uint32_t Resource::GetBufferIndex() const
{
    return m_BufferIndex;
}

void Resource::SetBufferIndex(std::uint32_t const &Offset)
{
    m_BufferIndex = Offset;
}

bool Resource::IsPendingDestroy() const
{
    return m_IsPendingDestroy;
}

void Resource::Destroy()
{
    m_IsPendingDestroy = true;
}
