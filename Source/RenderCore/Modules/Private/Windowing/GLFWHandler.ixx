// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>

export module RenderCore.Window.GLFWHandler;

export import <cstdint>;
export import <string_view>;

export import RenderCore.Window.Flags;

namespace RenderCore
{
    export class GLFWHandler final
    {
        GLFWwindow* m_Window {nullptr};

    public:
        [[nodiscard]] bool Initialize(std::uint16_t, std::uint16_t, std::string_view, InitializationFlags);
        void Shutdown();

        [[nodiscard]] GLFWwindow* GetWindow() const;
        [[nodiscard]] bool IsOpen() const;
    };
}// namespace RenderCore