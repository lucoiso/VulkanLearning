// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

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

export namespace RenderCore
{
    void InitializeEngine(GLFWwindow*);
    void ShutdownEngine();

    void DrawFrame(GLFWwindow*);
    [[nodiscard]] bool IsEngineInitialized();

    void LoadScene(std::string_view, std::string_view);
    void UnloadScene();

    [[nodiscard]] VkInstance& GetInstance();
    [[nodiscard]] VkSurfaceKHR& GetSurface();

    [[nodiscard]] glm::mat4 GetCameraMatrix();
    void SetCameraMatrix(glm::mat4 const&);
}// namespace RenderCore