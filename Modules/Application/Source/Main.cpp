// Copyright Notices: [...]

#include <RenderCore/Window.h>
#include <iostream>
#include <exception>
#include <memory>
#include <boost/log/trivial.hpp>

int main(int Argc, char* Argv[])
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Initializing application";

    try
    {
        if (const std::unique_ptr<RenderCore::Window> Window = std::make_unique<RenderCore::Window>();
            Window && Window->Initialize(800u, 600u, "Vulkan Project"))
        {
            while (Window->IsOpen())
            {
                Window->PollEvents();
            }
        }
    }
    catch (const std::exception& Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
