// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

#include <benchmark/benchmark.h>

// User defined modules
#include "RenderCore.hpp"

int main(int ArgC, char **ArgV)
{
    benchmark::Initialize(&ArgC, ArgV);
    benchmark::RunSpecifiedBenchmarks();

    return 0;
}
