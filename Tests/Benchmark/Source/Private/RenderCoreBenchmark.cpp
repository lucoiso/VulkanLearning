// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

#include <benchmark/benchmark.h>

// User defined modules
#include "RenderCore.hpp"

std::int32_t main(std::int32_t ArgC, char **ArgV)
{
    benchmark::Initialize(&ArgC, ArgV);
    benchmark::RunSpecifiedBenchmarks();

    return 0;
}
