// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

export module RenderCore.Unit.Utils;

import RenderCore.Window;
import RenderCore.Window.Flags;

export class ScopedWindow
{
    RenderCore::Window m_Window {};

public:
    ScopedWindow()
    {
        m_Window.Initialize(600U, 600U, "Vulkan Renderer: Tests", RenderCore::InitializationFlags::HEADLESS);
    }

    ~ScopedWindow()
    {
        m_Window.Shutdown();
    }

    RenderCore::Window& GetWindow()
    {
        return m_Window;
    }
};