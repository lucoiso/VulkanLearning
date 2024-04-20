// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <GLFW/glfw3.h>
#include <Volk/volk.h>
#include "RenderCoreModule.hpp"

export module RenderCore.Renderer;

import RenderCore.Types.RendererStateFlags;
import RenderCore.Types.Camera;
import RenderCore.Types.Illumination;
import RenderCore.Types.Transform;
import RenderCore.Types.Object;

import RenderCore.UserInterface.Control;
import RenderCore.Runtime.Memory;
import RenderCore.Runtime.Pipeline;
import RenderCore.Runtime.Command;
import RenderCore.Runtime.Device;

export namespace RenderCore
{
    void DrawFrame(GLFWwindow *, double, Control *);

    std::optional<std::int32_t> RequestImageIndex();

    void CheckObjectManagementFlags();

    void Tick();

    bool Initialize(GLFWwindow *);

    void Shutdown(GLFWwindow *);

    namespace Renderer
    {
        [[nodiscard]] RENDERCOREMODULE_API bool IsInitialized();

        [[nodiscard]] RENDERCOREMODULE_API bool IsReady();

        RENDERCOREMODULE_API void AddStateFlag(RendererStateFlags);

        RENDERCOREMODULE_API void RemoveStateFlag(RendererStateFlags);

        [[nodiscard]] RENDERCOREMODULE_API bool HasStateFlag(RendererStateFlags);

        [[nodiscard]] RENDERCOREMODULE_API RendererStateFlags GetStateFlags();

        RENDERCOREMODULE_API void RequestLoadObject(std::string_view);

        RENDERCOREMODULE_API void RequestUnloadObjects(std::vector<std::uint32_t> const &);

        RENDERCOREMODULE_API void RequestDestroyObjects();

        [[nodiscard]] RENDERCOREMODULE_API double GetFrameTime();

        RENDERCOREMODULE_API void SetFrameRateCap(double);

        [[nodiscard]] RENDERCOREMODULE_API double GetFrameRateCap();

        [[nodiscard]] RENDERCOREMODULE_API Camera const &GetCamera();

        [[nodiscard]] RENDERCOREMODULE_API Camera &GetMutableCamera();

        [[nodiscard]] RENDERCOREMODULE_API Illumination const &GetIllumination();

        [[nodiscard]] RENDERCOREMODULE_API Illumination &GetMutableIllumination();

        [[nodiscard]] RENDERCOREMODULE_API std::optional<std::int32_t> const &GetImageIndex();

        [[nodiscard]] RENDERCOREMODULE_API std::vector<std::shared_ptr<Object>> const &GetObjects();

        [[nodiscard]] RENDERCOREMODULE_API std::vector<std::shared_ptr<Object>> &GetMutableObjects();

        [[nodiscard]] RENDERCOREMODULE_API std::shared_ptr<Object> GetObjectByID(std::uint32_t);

        [[nodiscard]] RENDERCOREMODULE_API std::uint32_t GetNumObjects();

        [[nodiscard]] RENDERCOREMODULE_API VkSampler GetSampler();

        #ifdef VULKAN_RENDERER_ENABLE_IMGUI
        [[nodiscard]] RENDERCOREMODULE_API std::vector<VkImageView> GetViewportImages();

        [[nodiscard]] RENDERCOREMODULE_API bool IsImGuiInitialized();

        RENDERCOREMODULE_API void SaveFrameAsImage(std::string_view);
        #endif
    } // namespace Renderer
}     // namespace RenderCore
