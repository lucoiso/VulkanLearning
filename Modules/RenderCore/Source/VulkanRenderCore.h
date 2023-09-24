// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "Types/RenderStateFlags.h"
#include <string_view>
#include <vector>
#include <optional>
#include <volk.h>

struct GLFWwindow;

namespace RenderCore
{
    class VulkanRenderCore
    {
    public:
        VulkanRenderCore();

        VulkanRenderCore(const VulkanRenderCore &) = delete;
        VulkanRenderCore &operator=(const VulkanRenderCore &) = delete;

        ~VulkanRenderCore();

        static VulkanRenderCore &Get();

        void Initialize(GLFWwindow *const Window);
        void Shutdown();

        void DrawFrame(GLFWwindow *const Window);
        bool IsInitialized() const;

        void LoadScene(const std::string_view ModelPath, const std::string_view TexturePath);
        void UnloadScene();

        [[nodiscard]] VkInstance &GetInstance();
        [[nodiscard]] VkSurfaceKHR &GetSurface();

        VulkanRenderCoreStateFlags GetStateFlags() const;

    private:
        std::optional<std::int32_t> TryRequestDrawImage(GLFWwindow *const Window);

        void CreateVulkanInstance();
        void CreateVulkanSurface(GLFWwindow *const Window);
        void InitializeRenderCore(GLFWwindow *const Window);

        std::vector<VkPipelineShaderStageCreateInfo> CompileDefaultShaders();

        static VulkanRenderCore Instance;

        VkInstance m_Instance;
        VkSurfaceKHR m_Surface;
        mutable VulkanRenderCoreStateFlags StateFlags;

    #ifdef _DEBUG
        VkDebugUtilsMessengerEXT m_DebugMessenger;
    #endif
    };
}
