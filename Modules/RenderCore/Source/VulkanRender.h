// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "RenderCoreModule.h"
#include "Types/BufferRecordParameters.h"
#include <memory>
#include <string_view>
#include <vector>

struct GLFWwindow;

namespace RenderCore
{
    class VulkanRender
    {
    public:
        VulkanRender();

        VulkanRender(const VulkanRender &) = delete;
        VulkanRender &operator=(const VulkanRender &) = delete;

        ~VulkanRender();

        bool Initialize(GLFWwindow *const Window);
        void Shutdown();

        void DrawFrame(GLFWwindow *const Window);

        bool IsInitialized() const;

        void LoadScene(const std::string_view ModelPath, const std::string_view TexturePath);
        void UnloadScene();

    private:
        void CreateVulkanInstance();
        void CreateVulkanSurface(GLFWwindow *const Window);
        bool InitializeRenderCore(GLFWwindow *const Window);
        std::vector<VkPipelineShaderStageCreateInfo> CompileDefaultShaders();
        VulkanBufferRecordParameters GetBufferRecordParameters(const std::uint32_t ImageIndex, const std::uint64_t ObjectID) const;

        std::unique_ptr<class VulkanDeviceManager> m_DeviceManager;
        std::unique_ptr<class VulkanPipelineManager> m_PipelineManager;
        std::unique_ptr<class VulkanBufferManager> m_BufferManager;
        std::unique_ptr<class VulkanCommandsManager> m_CommandsManager;
        std::unique_ptr<class VulkanShaderManager> m_ShaderManager;

        VkInstance m_Instance;
        VkSurfaceKHR m_Surface;
        bool bIsSceneDirty;
        bool bIsSwapChainInvalidated;
        bool bHasLoadedScene;

    #ifdef _DEBUG
        VkDebugUtilsMessengerEXT m_DebugMessenger;
    #endif
    };
}
