// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

export module Benchmark.RenderCore;

import <benchmark/benchmark.h>;

import RenderCore.Window;

static void InitializeWindow(benchmark::State& State)
{
    for ([[maybe_unused]] auto const _: State)
    {
        if (RenderCore::Window Window; Window.Initialize(600U, 600U, "Vulkan Renderer", true).Get())
        {
            Window.Shutdown().Get();
        }
    }
}

BENCHMARK(InitializeWindow);
