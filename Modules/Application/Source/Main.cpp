// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include <RenderCore/Window.h>
#include <boost/log/trivial.hpp>
#include <QApplication>
#include <exception>
#include <memory>

int main(int Argc, char *Argv[])
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Initializing application";

    QApplication Application(Argc, Argv);
    Application.setApplicationName("Vulkan Project");
    Application.setApplicationVersion("0.0.1");
    Application.setOrganizationName("Lucas Vilas-Boas");

    try
    {
        const std::unique_ptr<RenderCore::Window> Window = std::make_unique<RenderCore::Window>();
        
        if (Window->Initialize(600u, 600u, "Vulkan Project"))
        {
            return Application.exec();
        }
    }
    catch (const std::exception &Ex)
    {
        BOOST_LOG_TRIVIAL(error) << "[Exception]: " << Ex.what();
    }

    return EXIT_FAILURE;
}
