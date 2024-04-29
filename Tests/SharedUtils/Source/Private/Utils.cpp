// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

#include "Utils.hpp"

import RenderCore.Renderer;

ScopedTestWindow::ScopedTestWindow()
{
    m_Window.Initialize(m_WindowWidth, m_WindowHeight, m_WindowTitle, m_WindowFlags);
    PollLoop([]
    {
        return !RenderCore::Renderer::IsReady();
    });
}

ScopedTestWindow::~ScopedTestWindow()
{
    m_Window.Shutdown();

    while (RenderCore::Renderer::IsInitialized())
    {
        m_Window.PollEvents();
    }
}

RenderCore::Window &ScopedTestWindow::GetWindow()
{
    return m_Window;
}

void ScopedTestWindow::PollLoop(std::uint8_t const Count)
{
    std::uint8_t Counter = 0U;
    do
    {
        m_Window.PollEvents();
        ++Counter;
    }
    while (Counter != Count);
}

void ScopedTestWindow::PollLoop(std::function<bool()> const &Condition)
{
    while (Condition())
    {
        m_Window.PollEvents();
    }
}
