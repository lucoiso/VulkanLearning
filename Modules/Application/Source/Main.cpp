// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include <RenderCore/Window.h>
#include <QApplication>
#include <memory>
#include <boost/log/trivial.hpp>

int main(int Argc, char *Argv[])
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Initializing application";

    QApplication Application(Argc, Argv);
    Application.setApplicationName("Vulkan Project");
    Application.setApplicationVersion("0.0.1");
    Application.setOrganizationName("Lucas Vilas-Boas");

    const std::unique_ptr<RenderCore::Window> Window = std::make_unique<RenderCore::Window>();

    if (Window->Initialize(600u, 600u, "Vulkan Project"))
    {
        return Application.exec();
    }

    return EXIT_FAILURE;
}
