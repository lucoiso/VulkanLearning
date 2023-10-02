// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include "RenderCoreModule.h"
#include <memory>
#include <string_view>

export module RenderCore.Window;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Window
    {
        class Impl;

    public:
        Window();

        Window(Window const&)            = delete;
        Window& operator=(Window const&) = delete;

        virtual ~Window();

        bool Initialize(std::uint16_t Width, std::uint16_t Height, std::string_view Title);
        void Shutdown() const;

        [[nodiscard]] bool IsInitialized() const;
        [[nodiscard]] bool IsOpen() const;

        void PollEvents() const;

        virtual void CreateOverlay();

    private:
        std::unique_ptr<Impl> m_Impl;
    };
}// namespace RenderCore