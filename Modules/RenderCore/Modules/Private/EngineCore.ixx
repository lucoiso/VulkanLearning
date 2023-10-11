// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <glm/ext.hpp>
#include <volk.h>

export module RenderCore.EngineCore;

import <array>;
import <cstdint>;
import <filesystem>;
import <optional>;
import <stdexcept>;
import <string_view>;
import <vector>;

namespace RenderCore
{
    export void InitializeEngine(GLFWwindow*);
    export void ShutdownEngine();

    export void DrawFrame(GLFWwindow*);
    export [[nodiscard]] bool IsEngineInitialized();

    export void LoadScene(std::string_view, std::string_view);
    export void UnloadScene();

    export [[nodiscard]] VkSurfaceKHR& GetSurface();
}// namespace RenderCore