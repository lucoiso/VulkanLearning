// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "Managers/VulkanRenderSubsystem.h"
#include "Managers/VulkanDeviceManager.h"
#include "Managers/VulkanPipelineManager.h"
#include "Managers/VulkanBufferManager.h"
#include "Managers/VulkanCommandsManager.h"
#include "Managers/VulkanShaderManager.h"
#include "Types/RenderStateFlags.h"
#include "Types/BufferRecordParameters.h"
#include <string_view>
#include <vector>
#include <optional>

class QQuickWindow;

namespace RenderCore
{
    class VulkanRender
    {
    public:
        VulkanRender();

        VulkanRender(const VulkanRender &) = delete;
        VulkanRender &operator=(const VulkanRender &) = delete;

        ~VulkanRender();

        void Initialize(const QQuickWindow *const Window);
        void Shutdown();

        void DrawFrame(const QQuickWindow *const Window);
        bool IsInitialized() const;

        void LoadScene(const std::string_view ModelPath, const std::string_view TexturePath);
        void UnloadScene();

    private:
        std::optional<std::int32_t> TryRequestDrawImage(const QQuickWindow *const Window);

        void CreateVulkanInstance();
        void CreateVulkanSurface(const QQuickWindow *const Window);
        void InitializeRenderCore(const QQuickWindow *const Window);

        std::vector<VkPipelineShaderStageCreateInfo> CompileDefaultShaders();
        VulkanBufferRecordParameters GetBufferRecordParameters(const std::uint32_t ImageIndex, const std::uint64_t ObjectID) const;

        VulkanDeviceManager m_DeviceManager;
        VulkanPipelineManager m_PipelineManager;
        VulkanBufferManager m_BufferManager;
        VulkanCommandsManager m_CommandsManager;
        VulkanShaderManager m_ShaderManager;

        VkInstance m_Instance;
        VkSurfaceKHR m_Surface;
        mutable VulkanRenderStateFlags StateFlags;

    #ifdef _DEBUG
        VkDebugUtilsMessengerEXT m_DebugMessenger;
    #endif
    };
}
