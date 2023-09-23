// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include <RenderCore/Window.h>
#include <QtQuick/QQuickView>
#include <QGuiApplication>
#include <memory>

int main(int Argc, char *Argv[])
{
    QGuiApplication Application(Argc, Argv);
    Application.setApplicationName("Vulkan Project");
    Application.setApplicationVersion("0.0.1");
    Application.setOrganizationName("Lucas Vilas-Boas");

    QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);

    const std::unique_ptr<RenderCore::Window> Window = std::make_unique<RenderCore::Window>();

    if (Window->Initialize(600u, 600u, "Vulkan Project"))
    {
        return Application.exec();
    }

    return EXIT_FAILURE;
}
