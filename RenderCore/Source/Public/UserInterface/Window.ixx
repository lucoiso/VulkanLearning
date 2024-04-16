// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <cstdint>
#include <string>
#include "RenderCoreModule.hpp"

export module RenderCore.UserInterface.Window;

import RenderCore.Renderer;
import RenderCore.UserInterface.Control;
import RenderCore.UserInterface.Window.Flags;
import RenderCore.Integrations.GLFWHandler;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Window : public Control
    {
        InitializationFlags m_Flags {};
        GLFWHandler         m_GLFWHandler {};
        std::uint16_t       m_Width {};
        std::uint16_t       m_Height {};
        std::string         m_Title {};

    public:
        Window();

        Window(Window const &) = delete;

        Window &operator=(Window const &) = delete;

        ~Window() override = default;

        bool Initialize(std::uint16_t, std::uint16_t, std::string_view, InitializationFlags Flags = InitializationFlags::NONE);

        void Shutdown();

        [[nodiscard]] bool IsOpen() const;

        virtual void PollEvents();

    protected:
        virtual void OnInitialized()
        {
        }

    private:
        void Draw();
    };
} // namespace RenderCore
