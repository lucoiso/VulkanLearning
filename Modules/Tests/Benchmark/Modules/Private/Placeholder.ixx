// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <benchmark/benchmark.h>

export module Benchmark.Placeholder;

static void Placeholder(benchmark::State& State)
{
    for ([[maybe_unused]] auto const _: State)
    {
    }
}

BENCHMARK(Placeholder);
