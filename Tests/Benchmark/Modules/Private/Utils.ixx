// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <string>

export module RenderCore.Unit.Utils;

import RenderCore.Window;
import RenderCore.Window.Flags;

export class ScopedWindow
{
    RenderCore::Window m_Window {};

    std::string const m_WindowTitle {"Vulkan Renderer: Tests"};
    std::uint16_t const m_WindowWidth {600U};
    std::uint16_t const m_WindowHeight {600U};
    RenderCore::InitializationFlags const m_WindowFlags {RenderCore::InitializationFlags::HEADLESS};

public:
    ScopedWindow() noexcept
    {
        m_Window.Initialize(m_WindowWidth, m_WindowHeight, m_WindowTitle, m_WindowFlags);
    }

    ~ScopedWindow() noexcept
    {
        m_Window.Shutdown();
    }

    RenderCore::Window& GetWindow()
    {
        return m_Window;
    }
};