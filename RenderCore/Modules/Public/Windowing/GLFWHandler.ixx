// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <cstdint>
#include <string>

export module RenderCore.Window.GLFWHandler;

import RenderCore.Window.Flags;

namespace RenderCore
{
    export class GLFWHandler final
    {
        GLFWwindow* m_Window {nullptr};

    public:
        [[nodiscard]] bool Initialize(std::uint16_t, std::uint16_t, std::string, InitializationFlags);
        void Shutdown();

        [[nodiscard]] GLFWwindow* GetWindow() const;
        [[nodiscard]] bool IsOpen() const;
    };
}// namespace RenderCore