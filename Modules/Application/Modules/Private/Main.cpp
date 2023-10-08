// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

import <memory>;
import RenderCore.Window;

int main([[maybe_unused]] int Argc, [[maybe_unused]] char* Argv[])
{
    if (auto const Window = std::make_unique<RenderCore::Window>();
        Window->Initialize(600U, 600U, "Vulkan Renderer"))
    {
        while (Window->IsOpen())
        {
            Window->PollEvents();
        }

        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}
