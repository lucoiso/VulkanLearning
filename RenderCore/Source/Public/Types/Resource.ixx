// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <string_view>
#include <vector>
#include <Volk/volk.h>

#include "RenderCoreModule.hpp"

export module RenderCore.Types.Resource;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Resource
    {
        bool          m_IsPendingDestroy { false };
        std::uint32_t m_ID {};
        std::string   m_Path {};
        std::string   m_Name {};
        std::uint32_t m_BufferIndex { 0U };

    public:
        virtual ~Resource() = default;
        Resource()          = delete;

        Resource(std::uint32_t, std::string_view);
        Resource(std::uint32_t, std::string_view, std::string_view);

        [[nodiscard]] std::uint32_t      GetID() const;
        [[nodiscard]] std::string const &GetPath() const;
        [[nodiscard]] std::string const &GetName() const;

        [[nodiscard]] std::uint32_t GetBufferIndex() const;
        void                        SetBufferIndex(std::uint32_t const &);

        [[nodiscard]] bool IsPendingDestroy() const;

        virtual void Destroy();
    };
} // namespace RenderCore
