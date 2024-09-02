// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.UserInterface.Window;

import RenderCore.Integrations.GLFWHandler;

export import RenderCore.Renderer;
export import RenderCore.UserInterface.Control;
export import RenderCore.UserInterface.Window.Flags;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Window : public Control
    {
        bool m_PendingClose {false};
        InitializationFlags m_Flags {};
        GLFWHandler         m_GLFWHandler {};
        std::uint16_t       m_Width {};
        std::uint16_t       m_Height {};
        strzilla::string    m_Title {};

    public:
        Window();

        Window(Window const &) = delete;

        Window &operator=(Window const &) = delete;

        ~Window() override = default;

        bool Initialize(std::uint16_t, std::uint16_t, strzilla::string_view, InitializationFlags Flags = InitializationFlags::NONE);

        void RequestClose();

        void Shutdown();

        [[nodiscard]] bool IsOpen() const;

        virtual void PollEvents();

    protected:
        virtual void OnInitialized()
        {
        }
    };
} // namespace RenderCore
