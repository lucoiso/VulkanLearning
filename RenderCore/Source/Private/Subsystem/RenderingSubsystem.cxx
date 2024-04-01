// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;


module RenderCore.Subsystem.Rendering;

using namespace RenderCore;

import RuntimeInfo.Manager;
import RenderCore.Utils.DebugHelpers;

RenderingSubsystem& RenderingSubsystem::Get()
{
    static RenderingSubsystem Instance{};
    return Instance;
}

void RenderingSubsystem::RegisterRenderer(Renderer *const Renderer)
{    m_RegisteredRenderer = Renderer;
}

void RenderingSubsystem::UnregisterRenderer()
{    m_RegisteredRenderer = nullptr;
}

Renderer* RenderingSubsystem::GetRenderer() const
{
    return m_RegisteredRenderer;
}
