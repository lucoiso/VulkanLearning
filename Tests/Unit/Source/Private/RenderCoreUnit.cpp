// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

#include <catch2/catch_all.hpp>

// User defined modules
#include "RenderCore.hpp"

int main(int const ArgC, char **ArgV)
{
    Catch::Session Session;

    if (int const Result = Session.applyCommandLine(ArgC, ArgV);
        Result != 0)
    {
        return Result;
    }

    return Session.run();
}
