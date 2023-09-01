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
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Starting polling events & drawing frames";
            {
                while (Window->IsOpen())
                {
                    Window->PollEvents();
                }
            }
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Window closed starting to freeing up resources";
        }
    }
    catch (const std::exception& Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
        return EXIT_FAILURE;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down application";
    return EXIT_SUCCESS;
}
