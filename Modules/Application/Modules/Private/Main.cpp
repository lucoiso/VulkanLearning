// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

import RenderCore.Window;

int main([[maybe_unused]] int Argc, [[maybe_unused]] char* Argv[])
{
    if (RenderCore::Window Window; Window.Initialize(600U, 600U, "Vulkan Renderer"))
    {
        while (Window.IsOpen())
        {
            Window.PollEvents();
        }

        return 0U;
    }

    return 1U;
}
