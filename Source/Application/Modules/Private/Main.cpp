// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>

import RenderCore.Window;

int main([[maybe_unused]] int Argc, [[maybe_unused]] char* Argv[])
{
#ifndef _DEBUG
    boost::log::core::get()->set_filter(boost::log::trivial::severity != boost::log::trivial::debug);
#endif

    if (RenderCore::Window Window; Window.Initialize(600U, 600U, "Vulkan Renderer 1", true, false).Get())
    {
        while (Window.IsOpen())
        {
            Window.PollEvents();
        }

        return 0U;
    }

    return 1U;
}
