// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Types.Resource;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Resource
    {
        bool          m_IsPendingDestroy {false};
        std::uint32_t m_ID {};
        strzilla::string   m_Path {};
        strzilla::string   m_Name {};
        std::uint32_t m_BufferIndex {0U};

    public:
        virtual ~Resource() = default;
                 Resource() = delete;

        Resource(std::uint32_t, strzilla::string_view);
        Resource(std::uint32_t, strzilla::string_view, strzilla::string_view);

        [[nodiscard]] std::uint32_t      GetID() const;
        [[nodiscard]] strzilla::string const &GetPath() const;
        [[nodiscard]] strzilla::string const &GetName() const;

        [[nodiscard]] std::uint32_t GetBufferIndex() const;
        void                        SetBufferIndex(std::uint32_t const &);

        [[nodiscard]] bool IsPendingDestroy() const;

        virtual void Destroy();
    };
} // namespace RenderCore
