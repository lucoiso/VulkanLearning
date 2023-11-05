// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <benchmark/benchmark.h>

export module Benchmark.RenderCore;

import RenderCore.Tests.SharedUtils;

import RenderCore.Window;
import RenderCore.Utils.RenderUtils;

static void InitializeWindow(benchmark::State& State)
{
    for ([[maybe_unused]] auto const _: State)
    {
        if (RenderCore::Window Window; Window.Initialize(600U, 600U, "Vulkan Renderer", true).Get())
        {
            Window.Shutdown().Get();
            benchmark::DoNotOptimize(Window);
        }
    }
}

BENCHMARK(InitializeWindow);

static void LoadAndUnloadScene(benchmark::State& State)
{
    ScopedWindow Window;
    benchmark::DoNotOptimize(Window);

    auto const& LoadedObjects = RenderCore::GetLoadedObjects();

    std::string DefaultObjectPath(LoadedObjects[0].GetPath());
    benchmark::DoNotOptimize(DefaultObjectPath);

    auto LoadedIDs = RenderCore::GetLoadedIDs();
    benchmark::DoNotOptimize(LoadedIDs);

    for ([[maybe_unused]] auto const _: State)
    {
        RenderCore::UnloadObject(LoadedIDs);
        LoadedIDs = RenderCore::LoadObject(DefaultObjectPath, "");
    }
}

BENCHMARK(LoadAndUnloadScene);