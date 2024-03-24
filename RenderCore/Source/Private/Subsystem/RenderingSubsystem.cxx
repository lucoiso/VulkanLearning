// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module RenderCore.Subsystem.Rendering;

using namespace RenderCore;

import RuntimeInfo.Manager;

RenderingSubsystem& RenderingSubsystem::Get()
{
    static RenderingSubsystem Instance{};
    return Instance;
}

void RenderingSubsystem::RegisterRenderer(Renderer *const Renderer)
{
    RuntimeInfo::Manager::Get().PushCallstack();
    m_RegisteredRenderer = Renderer;
}

void RenderingSubsystem::UnregisterRenderer()
{
    RuntimeInfo::Manager::Get().PushCallstack();
    m_RegisteredRenderer = nullptr;
}

Renderer* RenderingSubsystem::GetRenderer() const
{
    return m_RegisteredRenderer;
}
