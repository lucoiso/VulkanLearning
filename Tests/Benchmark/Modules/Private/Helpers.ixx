// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <benchmark/benchmark.h>

export module Benchmark.RenderCore.Helpers;

import RenderCore.Unit.Utils;

import RenderCore.Window;
import RenderCore.Utils.Helpers;

static void GetGLFWExtensions(benchmark::State& State)
{
    ScopedWindow Window;
    benchmark::DoNotOptimize(Window);

    for ([[maybe_unused]] auto const _: State)
    {
        [[maybe_unused]] auto Extensions = RenderCore::GetGLFWExtensions();
        benchmark::DoNotOptimize(Extensions);
    }
}

BENCHMARK(GetGLFWExtensions);

static void GetAvailableInstanceLayers(benchmark::State& State)
{
    ScopedWindow Window;
    benchmark::DoNotOptimize(Window);

    for ([[maybe_unused]] auto const _: State)
    {
        [[maybe_unused]] auto Layers = RenderCore::GetAvailableInstanceLayers();
        benchmark::DoNotOptimize(Layers);
    }
}

BENCHMARK(GetAvailableInstanceLayers);

static void GetAvailableInstanceLayersNames(benchmark::State& State)
{
    ScopedWindow Window;
    benchmark::DoNotOptimize(Window);

    for ([[maybe_unused]] auto const _: State)
    {
        [[maybe_unused]] auto Layers = RenderCore::GetAvailableInstanceLayersNames();
        benchmark::DoNotOptimize(Layers);
    }
}

BENCHMARK(GetAvailableInstanceLayersNames);

static void GetAvailableInstanceExtensions(benchmark::State& State)
{
    ScopedWindow Window;
    benchmark::DoNotOptimize(Window);

    for ([[maybe_unused]] auto const _: State)
    {
        [[maybe_unused]] auto Extensions = RenderCore::GetAvailableInstanceExtensions();
        benchmark::DoNotOptimize(Extensions);
    }
}

BENCHMARK(GetAvailableInstanceExtensions);

static void GetAvailableInstanceExtensionsNames(benchmark::State& State)
{
    ScopedWindow Window;
    benchmark::DoNotOptimize(Window);

    for ([[maybe_unused]] auto const _: State)
    {
        [[maybe_unused]] auto Extensions = RenderCore::GetAvailableInstanceExtensionsNames();
        benchmark::DoNotOptimize(Extensions);
    }
}

BENCHMARK(GetAvailableInstanceExtensionsNames);
