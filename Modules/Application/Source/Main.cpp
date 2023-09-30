// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

#include <RenderCore/Window.h>
#include <memory>

int main([[maybe_unused]] int Argc, [[maybe_unused]] char* Argv[])
{
    if (const auto Window = std::make_unique<RenderCore::Window>();
        Window->Initialize(600u, 600u, "Vulkan Project"))
    {
        while (Window->IsOpen())
        {
            Window->PollEvents();
        }

        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}
