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

        VulkanRenderCore(const VulkanRenderCore&)            = delete;
        VulkanRenderCore& operator=(const VulkanRenderCore&) = delete;

        ~VulkanRenderCore();

        static VulkanRenderCore& Get();

        void Initialize(GLFWwindow* Window);
        void Shutdown();

        void DrawFrame(GLFWwindow* Window) const;
        bool IsInitialized() const;

        void LoadScene(std::string_view ModelPath, std::string_view TexturePath);
        void UnloadScene() const;

        [[nodiscard]] VkInstance&   GetInstance();
        [[nodiscard]] VkSurfaceKHR& GetSurface();

        VulkanRenderCoreStateFlags GetStateFlags() const;

    private:
        std::optional<std::int32_t> TryRequestDrawImage(GLFWwindow* Window) const;

        void CreateVulkanInstance();
        void CreateVulkanSurface(GLFWwindow* Window);
        void InitializeRenderCore(GLFWwindow* Window) const;

        static std::vector<VkPipelineShaderStageCreateInfo> CompileDefaultShaders();

        static VulkanRenderCore g_Instance;

        VkInstance                         m_Instance;
        VkSurfaceKHR                       m_Surface;
        mutable VulkanRenderCoreStateFlags StateFlags;

        #ifdef _DEBUG
        VkDebugUtilsMessengerEXT m_DebugMessenger;
        #endif

        std::uint32_t m_ObjectID;
    };
}
