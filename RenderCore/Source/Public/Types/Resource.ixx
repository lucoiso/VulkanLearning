// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Types.Resource;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Resource
    {
        mutable bool     m_IsRenderDirty { true };
        bool             m_IsPendingDestroy { false };
        std::uint32_t    m_ID {};
        strzilla::string m_Path {};
        strzilla::string m_Name {};
        std::uint32_t    m_BufferIndex { 0U };

    public:
        virtual ~Resource() = default;
        Resource()          = delete;

        Resource(std::uint32_t, strzilla::string_view);
        Resource(std::uint32_t, strzilla::string_view, strzilla::string_view);

        [[nodiscard]] inline std::uint32_t GetID() const
        {
            return m_ID;
        }

        [[nodiscard]] inline strzilla::string const &GetPath() const
        {
            return m_Path;
        }

        [[nodiscard]] inline strzilla::string const &GetName() const
        {
            return m_Name;
        }

        [[nodiscard]] inline std::uint32_t GetBufferIndex() const
        {
            return m_BufferIndex;
        }

        inline void SetBufferIndex(std::uint32_t const &Offset)
        {
            m_BufferIndex = Offset;
        }

        [[nodiscard]] inline bool IsPendingDestroy() const
        {
            return m_IsPendingDestroy;
        }

        virtual inline void Destroy()
        {
            m_IsPendingDestroy = true;
        }

        [[nodiscard]] inline bool IsRenderDirty() const
        {
            return m_IsRenderDirty;
        }

        inline void MarkAsRenderDirty() const
        {
            m_IsRenderDirty = true;
        }

    protected:
        inline void SetRenderDirty(bool const Value) const
        {
            m_IsRenderDirty = Value;
        }
    };
} // namespace RenderCore
