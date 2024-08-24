// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

#include <catch2/catch_all.hpp>

// User defined modules
#include "RenderCore.hpp"

std::int32_t main(std::int32_t const ArgC, char **ArgV)
{
    Catch::Session Session;

    if (std::int32_t const Result = Session.applyCommandLine(ArgC, ArgV);
        Result != 0)
    {
        return Result;
    }

    return Session.run();
}
