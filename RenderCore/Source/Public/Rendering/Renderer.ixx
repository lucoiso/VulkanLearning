// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Renderer;

import RenderCore.Types.RendererStateFlags;
import RenderCore.Types.Camera;
import RenderCore.Types.Illumination;
import RenderCore.Types.Transform;
import RenderCore.Types.Object;
import RenderCore.Runtime.Memory;
import RenderCore.Runtime.Pipeline;
import RenderCore.Runtime.Command;
import RenderCore.Runtime.Device;
import RenderCore.UserInterface.Control;
import RenderCore.UserInterface.Window.Flags;

namespace RenderCore
{
    class Window;

    void Tick();

    export void DrawFrame(GLFWwindow *, double, Control *);
    export bool Initialize(GLFWwindow *, InitializationFlags);
    export void Shutdown(Control *);

    export namespace Renderer
    {
        [[nodiscard]] RENDERCOREMODULE_API bool IsInitialized();

        [[nodiscard]] RENDERCOREMODULE_API bool IsReady();

        RENDERCOREMODULE_API void AddStateFlag(RendererStateFlags);

        RENDERCOREMODULE_API void RemoveStateFlag(RendererStateFlags);

        [[nodiscard]] RENDERCOREMODULE_API bool HasStateFlag(RendererStateFlags);

        [[nodiscard]] RENDERCOREMODULE_API RendererStateFlags GetStateFlags();

        RENDERCOREMODULE_API void RequestLoadObject(strzilla::string_view);

        RENDERCOREMODULE_API void RequestUnloadObjects(std::vector<std::uint32_t> const &);

        RENDERCOREMODULE_API void RequestClearScene();

        RENDERCOREMODULE_API void RequestUpdateResources();

        [[nodiscard]] RENDERCOREMODULE_API double const &GetFrameTime();

        RENDERCOREMODULE_API void SetFPSLimit(double);

        [[nodiscard]] RENDERCOREMODULE_API double const &GetFPSLimit();

        [[nodiscard]] RENDERCOREMODULE_API bool const &GetVSync();

        RENDERCOREMODULE_API void SetVSync(bool);

        [[nodiscard]] RENDERCOREMODULE_API bool const &GetRenderOffscreen();

        RENDERCOREMODULE_API void SetRenderOffscreen(bool);

        [[nodiscard]] RENDERCOREMODULE_API Camera const &GetCamera();

        [[nodiscard]] RENDERCOREMODULE_API Camera &GetMutableCamera();

        [[nodiscard]] RENDERCOREMODULE_API Illumination const &GetIllumination();

        [[nodiscard]] RENDERCOREMODULE_API Illumination &GetMutableIllumination();

        [[nodiscard]] RENDERCOREMODULE_API std::uint32_t const &GetImageIndex();

        [[nodiscard]] RENDERCOREMODULE_API std::vector<std::shared_ptr<Object>> const &GetObjects();

        [[nodiscard]] RENDERCOREMODULE_API std::vector<std::shared_ptr<Object>> &GetMutableObjects();

        [[nodiscard]] RENDERCOREMODULE_API std::shared_ptr<Object> GetObjectByID(std::uint32_t);

        [[nodiscard]] RENDERCOREMODULE_API std::uint32_t GetNumObjects();

        [[nodiscard]] RENDERCOREMODULE_API VkSampler GetSampler();

        [[nodiscard]] RENDERCOREMODULE_API std::vector<VkImageView> GetOffscreenImages();

        [[nodiscard]] RENDERCOREMODULE_API bool IsImGuiInitialized();

        RENDERCOREMODULE_API void SaveOffscreenFrameToImage(strzilla::string_view);

        RENDERCOREMODULE_API void PrintMemoryAllocatorStats(bool);

        [[nodiscard]] RENDERCOREMODULE_API strzilla::string GetMemoryAllocatorStats(bool);

        [[nodiscard]] RENDERCOREMODULE_API InitializationFlags GetWindowInitializationFlags();

        [[nodiscard]] RENDERCOREMODULE_API std::uint8_t GetFrameIndex();
    } // namespace Renderer
}     // namespace RenderCore
