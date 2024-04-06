// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

#pragma once

#include <Utils.hpp>
#include <benchmark/benchmark.h>

import RenderCore.UserInterface.Window;
import RenderCore.UserInterface.Window.Flags;
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

    std::string const ObjectPath {"Resources/Assets/Box/glTF/Box.gltf"};
    benchmark::DoNotOptimize(ObjectPath);

    for ([[maybe_unused]] auto const _: State)
    {
        [[maybe_unused]] auto LoadedIDs = Window.GetWindow().GetRenderer().LoadScene(ObjectPath);
        benchmark::DoNotOptimize(LoadedIDs);

        Window.GetWindow().GetRenderer().UnloadAllScenes();
    }
}

BENCHMARK(LoadAndUnloadScene);