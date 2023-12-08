// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

export module RenderCore.Tests.SharedUtils;

import RenderCore.Window;
import RenderCore.Window.Flags;

export class ScopedWindow
{
    RenderCore::Window m_Window;

public:
    ScopedWindow() noexcept
    {
        try
        {
            m_Window.Initialize(600U, 600U, "Vulkan Renderer: Tests", RenderCore::InitializationFlags::HEADLESS).Get();
        }
        catch (...)
        {
        }
    }

    ~ScopedWindow() noexcept
    {
        try
        {
            m_Window.Shutdown().Get();
        }
        catch (...)
        {
        }
    }

    RenderCore::Window& GetWindow()
    {
        return m_Window;
    }
};