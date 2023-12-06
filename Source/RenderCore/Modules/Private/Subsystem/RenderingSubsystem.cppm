// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>

module RenderCore.Subsystem.Rendering;

using namespace RenderCore;

import Timer.ExecutionCounter;

RenderingSubsystem& RenderingSubsystem::Get()
{
    static RenderingSubsystem Instance {};
    return Instance;
}

void RenderingSubsystem::RegisterRenderer(GLFWwindow* Window, Renderer& Renderer)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    if (!Window)
    {
        return;
    }

    m_RegisteredRenderers.emplace(Window, &Renderer);
}

void RenderingSubsystem::UnregisterRenderer(GLFWwindow* const Window)
{
    Timer::ScopedTimer TotalSceneAllocationTimer(__FUNCTION__);

    if (!m_RegisteredRenderers.contains(Window))
    {
        return;
    }

    m_RegisteredRenderers.erase(Window);
}

Renderer* RenderingSubsystem::GetRenderer(GLFWwindow* const Window) const
{
    if (!m_RegisteredRenderers.contains(Window))
    {
        return nullptr;
    }

    return m_RegisteredRenderers.at(Window);
}