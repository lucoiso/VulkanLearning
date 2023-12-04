// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <RenderCoreModule.h>

export module RenderCore.Window;

export import <cstdint>;
export import <string_view>;
export import <memory>;

export import Coroutine.Types;
export import RenderCore.Renderer;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Window
    {
        class WindowImpl;
        std::unique_ptr<WindowImpl> m_WindowImpl {nullptr};

        Renderer m_Renderer {};

    public:
        Window();

        Window(Window const&)            = delete;
        Window& operator=(Window const&) = delete;

        virtual ~Window();

        enum class InitializationFlags : std::uint8_t
        {
            NONE        = 0,
            MAXIMIZED   = 1 << 0,
            HEADLESS    = 1 << 1,
        };

        AsyncOperation<bool> Initialize(std::uint16_t, std::uint16_t, std::string_view const&, InitializationFlags Flags = InitializationFlags::NONE);
        AsyncTask Shutdown();

        [[nodiscard]] bool IsInitialized() const;
        [[nodiscard]] bool IsOpen() const;

        [[nodiscard]] Renderer& GetRenderer();

        virtual void PollEvents();

    protected:
        virtual void CreateOverlay();

    private:
        void RequestRender();
    };
}// namespace RenderCore