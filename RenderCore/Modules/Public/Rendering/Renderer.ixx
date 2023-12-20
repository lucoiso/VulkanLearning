// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include "RenderCoreModule.hpp"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <volk.h>

export module RenderCore.Renderer;

import RenderCore.Types.Camera;
import RenderCore.Types.Transform;
import RenderCore.Types.Object;
import Timer.Manager;

import RenderCore.Utils.EngineStateFlags;
import RenderCore.Management.BufferManagement;
import RenderCore.Management.PipelineManagement;
import RenderCore.Management.CommandsManagement;
import RenderCore.Management.DeviceManagement;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Renderer
    {
        BufferManager m_BufferManager {};
        PipelineManager m_PipelineManager {};
        Camera m_Camera {};
        ViewSize m_ViewportOffset {};

        RendererStateFlags m_StateFlags {RendererStateFlags::NONE};
        std::vector<std::shared_ptr<Object>> m_Objects {};
        double m_DeltaTime {0.F};
        double m_FrameTime {0.F};
        double m_FrameRateCap {0.016667F};
        std::mutex m_ObjectsMutex {};

        friend class Window;

        void DrawFrame(GLFWwindow*, Camera const&, std::function<void()>&&);
        std::optional<std::int32_t> RequestImageIndex(GLFWwindow*);

        void Tick();
        void RemoveInvalidObjects();

        bool Initialize(GLFWwindow*);
        void Shutdown(GLFWwindow*);

    public:
        Renderer()  = default;
        ~Renderer() = default;

        [[nodiscard]] bool IsInitialized() const;

        void AddStateFlag(RendererStateFlags);
        void RemoveStateFlag(RendererStateFlags);
        [[nodiscard]] bool HasStateFlag(RendererStateFlags) const;

        [[nodiscard]] std::vector<std::uint32_t> LoadScene(std::string_view const&);
        void UnloadScene(std::vector<std::uint32_t> const&);
        void UnloadAllScenes();

        static [[nodiscard]] Timer::Manager& GetRenderTimerManager();
        [[nodiscard]] double GetDeltaTime() const;
        [[nodiscard]] double GetFrameTime() const;

        void SetFrameRateCap(double);
        [[nodiscard]] double GetFrameRateCap() const;

        [[nodiscard]] Camera const& GetCamera() const;
        [[nodiscard]] Camera& GetMutableCamera();

        [[nodiscard]] ViewSize const& GetViewportSize() const;
        void SetViewportOffset(ViewSize const&);

        [[nodiscard]] std::vector<std::shared_ptr<Object>> const& GetObjects() const;
        [[nodiscard]] std::shared_ptr<Object> GetObjectByID(std::uint32_t) const;
        [[nodiscard]] std::uint32_t GetNumObjects() const;

        [[nodiscard]] VkImageView GetSwapChainImageView(std::uint32_t) const;
        [[nodiscard]] VkSampler GetSwapChainSampler(std::uint32_t) const;
    };
}// namespace RenderCore