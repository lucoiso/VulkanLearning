// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

#include <benchmark/benchmark.h>

#ifndef _DEBUG
#endif

// User defined modules
#include "Helpers.hpp"
#include "RenderCore.hpp"

int main(int ArgC, char** ArgV)
{
#ifndef _DEBUG
    boost::log::core::get()->set_logging_enabled(false);
#endif

    benchmark::Initialize(&ArgC, ArgV);
    benchmark::RunSpecifiedBenchmarks();

    return 0;
}
