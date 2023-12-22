// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <functional>
#include <volk.h>

export module RenderCore.Management.ImGuiManagement;

import RenderCore.Management.BufferManagement;
import RenderCore.Management.PipelineManagement;
import RenderCore.Types.SurfaceProperties;

namespace RenderCore
{
    export void InitializeImGuiContext(GLFWwindow*);

    export void CreateImGuiRenderPass(PipelineManager const&, SurfaceProperties const&);
    export void DestroyImGuiRenderPass();

    export void CreateImGuiFrameBuffers(BufferManager const& BufferManager);
    export void DestroyImGuiFrameBuffers();

    export void ReleaseImGuiResources();
    export void DrawImGuiFrame(std::function<void()>&&, std::function<void()>&&, std::function<void()>&&);

    export [[nodiscard]] bool IsImGuiInitialized();

    export [[nodiscard]] VkRenderPass const& GetImGuiRenderPass();
    export [[nodiscard]] std::vector<VkFramebuffer> const& GetImGuiFrameBuffers();
}// namespace RenderCore