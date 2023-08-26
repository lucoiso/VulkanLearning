// Copyright Notices: [...]

#include <RenderCore/Window.h>
#include <iostream>
#include <exception>
#include <memory>

int main(int Argc, char* Argv[])
{
    try
    {
        const std::unique_ptr<RenderCore::Window> Window = std::make_unique<RenderCore::Window>();
        Window->Initialize(800u, 600u, "Vulkan Project");

        while (Window->IsOpen())
        {
            Window->PollEvents();
        }
    }
    catch (const std::exception& Exception)
    {
        std::cerr << Exception.what();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
