// Copyright Notices: [...]

#pragma once

#include <benchmark/benchmark.h>
#include <RenderCore/Window.h>

static void OpenWindow(benchmark::State& state)
{
    for (const auto& _ : state)
    {
        if (const std::unique_ptr<RenderCore::Window> Window = std::make_unique<RenderCore::Window>();
            Window && Window->Initialize(800u, 600u, "Vulkan Project"))
        {
            Window->PollEvents();
        }
    }
}
BENCHMARK(OpenWindow);