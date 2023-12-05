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
export import RenderCore.Window.Flags;
import RenderCore.Window.GLFWHandler;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Window
    {
        GLFWHandler m_GLFWHandler {};
        Renderer m_Renderer {};

    public:
        Window() = default;

        Window(Window const&)            = delete;
        Window& operator=(Window const&) = delete;

        virtual ~Window();

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