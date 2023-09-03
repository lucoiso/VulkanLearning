// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include <benchmark/benchmark.h>

static void Placeholder(benchmark::State &state)
{
    for (const auto &_ : state)
    {
        continue;
    }
}
BENCHMARK(Placeholder);