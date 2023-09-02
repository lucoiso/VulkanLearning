// Copyright Notices: [...]

#pragma once

#include <benchmark/benchmark.h>

static void Placeholder(benchmark::State& state)
{
    for (const auto& _ : state)
    {
        continue;
    }
}
BENCHMARK(Placeholder);