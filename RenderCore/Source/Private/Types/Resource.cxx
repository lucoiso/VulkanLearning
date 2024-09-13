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
