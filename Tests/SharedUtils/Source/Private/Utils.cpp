// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

#include "Utils.hpp"

ScopedWindow::ScopedWindow()
{
    m_Window.Initialize(m_WindowWidth, m_WindowHeight, m_WindowTitle, m_WindowFlags);
}

ScopedWindow::~ScopedWindow()
{
    m_Window.Shutdown();
}

RenderCore::Window& ScopedWindow::GetWindow()
{
    return m_Window;
}