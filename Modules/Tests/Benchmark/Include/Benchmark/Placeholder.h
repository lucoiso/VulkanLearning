// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

#pragma once

#include <benchmark/benchmark.h>

static void Placeholder(benchmark::State& State)
{
    for (auto const _ : State)
    {
    }
}

BENCHMARK(Placeholder);
