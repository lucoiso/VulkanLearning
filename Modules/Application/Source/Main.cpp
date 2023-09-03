// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include <RenderCore/Window.h>
#include <boost/log/trivial.hpp>
#include <exception>
#include <memory>

int main(int Argc, char *Argv[])
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Initializing application";

    try
    {
        if (const std::unique_ptr<RenderCore::Window> Window = std::make_unique<RenderCore::Window>();
            Window && Window->Initialize(600u, 600u, "Vulkan Project"))
        {
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Starting polling events & drawing frames";
            {
                while (Window->IsOpen())
                {
                    Window->PollEvents();
                }
            }
            BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Window closed. Starting to freeing up resources";
        }
    }
    catch (const std::exception &Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
        return EXIT_FAILURE;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down application";
    return EXIT_SUCCESS;
}
