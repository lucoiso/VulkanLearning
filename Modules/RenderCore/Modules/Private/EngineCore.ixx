// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <glm/ext.hpp>
#include <volk.h>

export module RenderCore.EngineCore;

export import <cstdint>;
export import <list>;
export import <vector>;
export import <string_view>;

namespace RenderCore
{
    export void InitializeEngine(GLFWwindow*);
    export void ShutdownEngine();

    export void DrawFrame(GLFWwindow*);
    export [[nodiscard]] bool IsEngineInitialized();

    export [[nodiscard]] std::list<std::uint32_t> LoadScene(std::string_view, std::string_view);
    export void UnloadScene(std::list<std::uint32_t> const&);

    export [[nodiscard]] std::vector<class Object> const& GetObjects();
    export [[nodiscard]] VkSurfaceKHR& GetSurface();
}// namespace RenderCore