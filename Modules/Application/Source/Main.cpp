// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

#include <RenderCore/Window.h>
#include <memory>

int main(int Argc, char* Argv[])
{
    const auto Window = std::make_unique<RenderCore::Window>();
    if (Window->Initialize(600u, 600u, "Vulkan Project"))
    {
        while (Window->IsOpen())
        {
            Window->PollEvents();
        }

        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}
