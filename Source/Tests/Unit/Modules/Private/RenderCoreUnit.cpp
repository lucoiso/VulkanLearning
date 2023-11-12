// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

import Unit.RenderCore;
import Unit.RenderCore.Helpers;

#include <boost/log/trivial.hpp>
#include <catch2/catch_all.hpp>

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
