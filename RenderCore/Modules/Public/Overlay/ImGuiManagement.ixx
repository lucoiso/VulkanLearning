// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <functional>
#include <volk.h>

export module RenderCore.Management.ImGuiManagement;

import RenderCore.Management.DeviceManagement;
import RenderCore.Management.BufferManagement;
import RenderCore.Types.SurfaceProperties;

namespace RenderCore
{
    export void InitializeImGui(GLFWwindow*);
    export void CreateImGuiRenderPass(VkFormat const& Format);
    export void CreateImGuiFrameBuffers(BufferManager const& BufferManager);
    export void ReleaseImGuiResources();
    export void DrawImGuiFrame(std::function<void()>&&, std::function<void()>&&, std::function<void()>&&);

    export [[nodiscard]] bool IsImGuiInitialized();

    export [[nodiscard]] VkRenderPass const& GetImGuiRenderPass();
    export [[nodiscard]] std::vector<VkFramebuffer> const& GetImGuiFrameBuffers();
}// namespace RenderCore