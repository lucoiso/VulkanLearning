// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <cstdint>
#include <string>
#include "RenderCoreModule.hpp"

export module RenderCore.Window;

import RenderCore.Renderer;
import RenderCore.Window.Control;
import RenderCore.Window.Flags;
import RenderCore.Window.GLFWHandler;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Window : public Control
    {
        GLFWHandler m_GLFWHandler{};
        Renderer    m_Renderer{};

        std::string         m_Title{};
        std::uint16_t       m_Width{};
        std::uint16_t       m_Height{};
        InitializationFlags m_Flags{};

    public:
        Window();

        Window(Window const&) = delete;

        Window& operator=(Window const&) = delete;

        ~Window() override;

        bool Initialize(std::uint16_t,
                        std::uint16_t,
                        std::string_view,
                        InitializationFlags Flags = InitializationFlags::NONE);

        void Shutdown();

        [[nodiscard]] bool IsInitialized() const;

        [[nodiscard]] bool IsOpen() const;

        [[nodiscard]] Renderer& GetRenderer();

        virtual void PollEvents();

    protected:
        virtual void OnInitialized()
        {
        }

    private:
        void RequestRender(float);
    };
} // namespace RenderCore
