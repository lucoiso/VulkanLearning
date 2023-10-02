// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

export module Benchmark.Placeholder;

#include "benchmark/benchmark.h"

static void Placeholder(benchmark::State& State)
{
    for ([[maybe_unused]] auto const _: State)
    {
    }
}

BENCHMARK(Placeholder);
