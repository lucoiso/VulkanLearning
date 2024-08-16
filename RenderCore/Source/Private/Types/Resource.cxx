// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.Types.Resource;

using namespace RenderCore;

strzilla::string NameParser(strzilla::string_view const Path)
{
    auto const LastSlash = Path.rfind('\\');
    return LastSlash != strzilla::string::npos ? Path.substr(LastSlash + 1, Path.size() - LastSlash - 1) : "";
}

Resource::Resource(std::uint32_t const ID, strzilla::string_view const Path)
    : m_ID(ID)
  , m_Path(Path)
  , m_Name(NameParser(m_Path))
{
}

Resource::Resource(std::uint32_t const ID, strzilla::string_view const Path, strzilla::string_view const Name)
    : m_ID(ID)
  , m_Path(Path)
  , m_Name(Name)
{
}

std::uint32_t Resource::GetID() const
{
    return m_ID;
}

strzilla::string const &Resource::GetPath() const
{
    return m_Path;
}

strzilla::string const &Resource::GetName() const
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
