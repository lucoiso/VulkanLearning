// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

#include <catch2/catch_all.hpp>

// User defined modules
#include "Helpers.hpp"
#include "RenderCore.hpp"

#ifndef _DEBUG
#endif

int main(int const ArgC, char** ArgV)
{
#ifndef _DEBUG
    boost::log::core::get()->set_logging_enabled(false);
#endif

    Catch::Session Session;

    if (int const Result = Session.applyCommandLine(ArgC, ArgV); Result != 0)
    {
        return Result;
    }

    return Session.run();
}
