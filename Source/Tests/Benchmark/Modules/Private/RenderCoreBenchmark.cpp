// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

#include <benchmark/benchmark.h>
#include <boost/log/trivial.hpp>

import Benchmark.RenderCore;
import Benchmark.RenderCore.Helpers;

int main(int ArgC, char** ArgV)
{
#ifndef _DEBUG
    boost::log::core::get()->set_logging_enabled(false);
#endif

    benchmark::Initialize(&ArgC, ArgV);
    benchmark::RunSpecifiedBenchmarks();

    return 0;
}
