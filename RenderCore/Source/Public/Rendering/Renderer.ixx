// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <Volk/volk.h>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include "RenderCoreModule.hpp"

export module RenderCore.Renderer;

import Timer.Manager;

import RenderCore.Types.RendererStateFlags;
import RenderCore.Types.Camera;
import RenderCore.Types.Illumination;
import RenderCore.Types.Transform;
import RenderCore.Types.Object;

import RenderCore.UserInterface.Control;
import RenderCore.Runtime.Buffer;
import RenderCore.Runtime.Pipeline;
import RenderCore.Runtime.Command;
import RenderCore.Runtime.Device;

namespace RenderCore
{
    export class RENDERCOREMODULE_API Renderer
    {
        Camera       m_Camera {};
        Illumination m_Illumination {};

        RendererStateFlags          m_StateFlags {RendererStateFlags::NONE};
        double                      m_DeltaTime {0.F};
        double                      m_FrameTime {0.F};
        double                      m_FrameRateCap {0.016667F};
        std::optional<std::int32_t> m_ImageIndex {};
        std::mutex                  m_RenderingMutex {};

        friend class Window;

        void DrawFrame(GLFWwindow *, float, Camera const &, Control *);

        std::optional<std::int32_t> RequestImageIndex(GLFWwindow *);

        void Tick();

        void RemoveInvalidObjects();

        bool Initialize(GLFWwindow *);

        void Shutdown(GLFWwindow *);

    public:
        Renderer() = default;

        ~Renderer() = default;

        [[nodiscard]] bool IsInitialized() const;

        [[nodiscard]] bool IsReady() const;

        void AddStateFlag(RendererStateFlags);

        void RemoveStateFlag(RendererStateFlags);

        [[nodiscard]] bool HasStateFlag(RendererStateFlags) const;

        [[nodiscard]] RendererStateFlags GetStateFlags() const;

        void LoadScene(std::string_view);

        void UnloadScene(std::vector<std::uint32_t> const &);

        void UnloadAllScenes();

        static [[nodiscard]] Timer::Manager &GetRenderTimerManager();

        [[nodiscard]] double GetDeltaTime() const;

        [[nodiscard]] double GetFrameTime() const;

        void SetFrameRateCap(double);

        [[nodiscard]] double GetFrameRateCap() const;

        [[nodiscard]] Camera const &GetCamera() const;

        [[nodiscard]] Camera &GetMutableCamera();

        [[nodiscard]] Illumination const &GetIllumination() const;

        [[nodiscard]] Illumination &GetMutableIllumination();

        [[nodiscard]] std::optional<std::int32_t> const &GetImageIndex() const;

        [[nodiscard]] std::vector<std::shared_ptr<Object>> const &GetObjects() const;

        [[nodiscard]] std::shared_ptr<Object> GetObjectByID(std::uint32_t) const;

        [[nodiscard]] std::uint32_t GetNumObjects() const;

        [[nodiscard]] VkSampler GetSampler() const;

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
        [[nodiscard]] std::vector<VkImageView> GetViewportRenderImageViews() const;

        static [[nodiscard]] bool IsImGuiInitialized();

        void SaveFrameToImageFile(std::string_view) const;
#endif
    };
} // namespace RenderCore
