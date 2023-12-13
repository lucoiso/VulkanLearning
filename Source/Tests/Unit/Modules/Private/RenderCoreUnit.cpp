// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

#include <catch2/catch_all.hpp>

#ifndef _DEBUG
#include <boost/log/trivial.hpp>
#endif

import Unit.RenderCore;
import Unit.RenderCore.Helpers;

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
