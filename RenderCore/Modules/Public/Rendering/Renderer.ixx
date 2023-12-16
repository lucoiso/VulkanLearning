// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include "RenderCoreModule.h"
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>
#include <optional>
#include <string>

export module RenderCore.Renderer;

import RenderCore.Types.Camera;
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

        RendererStateFlags m_StateFlags {RendererStateFlags::NONE};
        std::vector<std::shared_ptr<Object>> m_Objects {};
        double m_DeltaTime {0.F};
        double m_FrameTime {0.F};
        double m_FrameRateCap {0.016667F};
        std::mutex m_ObjectsMutex {};

        friend class Window;

        void DrawFrame(GLFWwindow*, Camera const&);
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

        [[nodiscard]] std::vector<std::uint32_t> LoadScene(std::string const&);
        void UnloadScene(std::vector<std::uint32_t> const&);
        void UnloadAllScenes();

        static [[nodiscard]] Timer::Manager& GetRenderTimerManager();
        [[nodiscard]] double GetDeltaTime() const;
        [[nodiscard]] double GetFrameTime() const;

        void SetFrameRateCap(double);
        [[nodiscard]] double GetFrameRateCap() const;

        [[nodiscard]] Camera const& GetCamera() const;
        [[nodiscard]] Camera& GetMutableCamera();
        [[nodiscard]] std::vector<std::shared_ptr<Object>> const& GetObjects() const;
        [[nodiscard]] std::shared_ptr<Object> GetObjectByID(std::uint32_t) const;
        [[nodiscard]] std::uint32_t GetNumObjects() const;
    };
}// namespace RenderCore