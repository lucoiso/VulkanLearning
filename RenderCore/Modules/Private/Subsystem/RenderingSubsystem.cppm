// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module RenderCore.Subsystem.Rendering;

using namespace RenderCore;

import Timer.ExecutionCounter;

RenderingSubsystem& RenderingSubsystem::Get()
{
    static RenderingSubsystem Instance {};
    return Instance;
}

void RenderingSubsystem::RegisterRenderer(Renderer* const Renderer)
{
    m_RegisteredRenderer = Renderer;
}

void RenderingSubsystem::UnregisterRenderer()
{
    m_RegisteredRenderer = nullptr;
}

Renderer* RenderingSubsystem::GetRenderer() const
{
    return m_RegisteredRenderer;
}
