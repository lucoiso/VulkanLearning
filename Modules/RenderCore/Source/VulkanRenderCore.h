// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

#pragma once

#include "Types/RenderStateFlags.h"
#include <optional>
#include <string_view>
#include <vector>
#include <volk.h>

struct GLFWwindow;

namespace RenderCore
{
    class VulkanRenderCore final
    {
    public:
        VulkanRenderCore();

        VulkanRenderCore(VulkanRenderCore const&)            = delete;
        VulkanRenderCore& operator=(VulkanRenderCore const&) = delete;

        ~VulkanRenderCore();

        static VulkanRenderCore& Get();

        void Initialize(GLFWwindow* Window);
        void Shutdown();

        void DrawFrame(GLFWwindow* Window) const;
        bool IsInitialized() const;

        void LoadScene(std::string_view ModelPath, std::string_view TexturePath);
        void UnloadScene() const;

        [[nodiscard]] VkInstance& GetInstance();
        [[nodiscard]] VkSurfaceKHR& GetSurface();

        VulkanRenderCoreStateFlags GetStateFlags() const;

    private:
        std::optional<std::int32_t> TryRequestDrawImage() const;

        void CreateVulkanInstance();
        void CreateVulkanSurface(GLFWwindow* Window);
        void InitializeRenderCore() const;

        static std::vector<VkPipelineShaderStageCreateInfo> CompileDefaultShaders();

        VkInstance m_Instance;
        VkSurfaceKHR m_Surface;
        mutable VulkanRenderCoreStateFlags m_StateFlags;
        std::uint64_t m_ObjectID;

#ifdef _DEBUG
        VkDebugUtilsMessengerEXT m_DebugMessenger;
#endif
    };
}// namespace RenderCore
