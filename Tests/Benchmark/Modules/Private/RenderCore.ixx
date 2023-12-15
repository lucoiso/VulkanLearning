// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <benchmark/benchmark.h>

export module Benchmark.RenderCore;

import RenderCore.Unit.Utils;

import RenderCore.Window;
import RenderCore.Window.Flags;
import RenderCore.Renderer;

static void InitializeWindow(benchmark::State& State)
{
    for ([[maybe_unused]] auto const _: State)
    {
        RenderCore::Window Window;
        benchmark::DoNotOptimize(Window);

        if (Window.Initialize(600U, 600U, "Vulkan Renderer", RenderCore::InitializationFlags::HEADLESS))
        {
            Window.Shutdown();
        }
    }
}

BENCHMARK(InitializeWindow);

static void LoadAndUnloadScene(benchmark::State& State)
{
    ScopedWindow Window;
    benchmark::DoNotOptimize(Window);

    std::string ObjectPath {"Resources/Assets/Box/glTF/Box.gltf"};
    benchmark::DoNotOptimize(ObjectPath);

    for ([[maybe_unused]] auto const _: State)
    {
        [[maybe_unused]] auto LoadedIDs = Window.GetWindow().GetRenderer().LoadScene(ObjectPath);
        benchmark::DoNotOptimize(LoadedIDs);

        Window.GetWindow().GetRenderer().UnloadAllScenes();
    }
}

BENCHMARK(LoadAndUnloadScene);