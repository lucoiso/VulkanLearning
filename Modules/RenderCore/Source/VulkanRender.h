// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "Types/RenderStateFlags.h"
#include "Types/BufferRecordParameters.h"
#include <string_view>
#include <vector>
#include <optional>
#include <GLFW/glfw3.h>

namespace RenderCore
{
    class VulkanRender
    {
    public:
        VulkanRender();

        VulkanRender(const VulkanRender &) = delete;
        VulkanRender &operator=(const VulkanRender &) = delete;

        ~VulkanRender();

        static VulkanRender &Get();

        void Initialize(GLFWwindow *const Window);
        void Shutdown();

        void DrawFrame(GLFWwindow *const Window);
        bool IsInitialized() const;

        void LoadScene(const std::string_view ModelPath, const std::string_view TexturePath);
        void UnloadScene();

        [[nodiscard]] VkInstance &GetInstance();
        [[nodiscard]] VkSurfaceKHR &GetSurface();

        VulkanRenderStateFlags GetStateFlags() const;

    private:
        std::optional<std::int32_t> TryRequestDrawImage(GLFWwindow *const Window);

        void CreateVulkanInstance();
        void CreateVulkanSurface(GLFWwindow *const Window);
        void InitializeRenderCore(GLFWwindow *const Window);

        std::vector<VkPipelineShaderStageCreateInfo> CompileDefaultShaders();
        VulkanBufferRecordParameters GetBufferRecordParameters(const std::uint32_t ImageIndex, const std::uint64_t ObjectID, GLFWwindow *const Window) const;

        static VulkanRender Instance;

        VkInstance m_Instance;
        VkSurfaceKHR m_Surface;
        mutable VulkanRenderStateFlags StateFlags;

    #ifdef _DEBUG
        VkDebugUtilsMessengerEXT m_DebugMessenger;
    #endif
    };
}
