// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Renderer;

import RenderCore.Utils.Constants;
import RenderCore.Utils.EnumHelpers;
import RenderCore.Types.Object;
import RenderCore.Types.Texture;
import RenderCore.Types.RendererStateFlags;
import RenderCore.Runtime.SwapChain;

namespace RenderCore
{
    RENDERCOREMODULE_API auto g_StateFlags { RendererStateFlags::NONE };
    RENDERCOREMODULE_API auto g_ObjectsManagementStateFlags { RendererObjectsManagementStateFlags::NONE };
    RENDERCOREMODULE_API float                             g_FrameTime { 0.F };
    RENDERCOREMODULE_API float                             g_FrameRateCap { 0.016667F };
    RENDERCOREMODULE_API bool                              g_UseVSync { true };
    RENDERCOREMODULE_API bool                              g_RenderOffscreen { false };
    RENDERCOREMODULE_API bool                              g_UseDefaultSync { true };
    RENDERCOREMODULE_API std::uint32_t                     g_ImageIndex { g_ImageCount };
    RENDERCOREMODULE_API std::mutex                        g_RendererMutex {};
    RENDERCOREMODULE_API std::queue<std::function<void()>> g_MainThreadDispatchQueue {};
    RENDERCOREMODULE_API std::queue<std::function<void()>> g_NextTickDispatchQueue {};

    RENDERCOREMODULE_API std::vector<strzilla::string> g_ModelsToLoad {};
    RENDERCOREMODULE_API std::vector<std::uint32_t> g_ModelsToUnload {};

    std::function<void()> g_OnInitializeCallback {};
    std::function<void()> g_OnRefreshCallback {};
    std::function<void()> g_OnDrawCallback {};
    std::function<void()> g_OnShutdownCallback {};
}

namespace RenderCore
{
    export namespace Renderer
    {
        RENDERCOREMODULE_API void Tick();
        RENDERCOREMODULE_API void DrawFrame(GLFWwindow *, double);

        RENDERCOREMODULE_API void SetOnInitializeCallback(std::function<void()> &&);
        RENDERCOREMODULE_API void SetOnRefreshCallback(std::function<void()> &&);
        RENDERCOREMODULE_API void SetOnDrawCallback(std::function<void()> &&);
        RENDERCOREMODULE_API void SetOnShutdownCallback(std::function<void()> &&);

        RENDERCOREMODULE_API [[nodiscard]] bool Initialize(GLFWwindow *);
        RENDERCOREMODULE_API void               Shutdown();

        RENDERCOREMODULE_API void DispatchToMainThread(std::function<void()> &&);
        RENDERCOREMODULE_API void DispatchToNextTick(std::function<void()> &&);

        RENDERCOREMODULE_API void SetVSync(bool);
        RENDERCOREMODULE_API void SetRenderOffscreen(bool);
        RENDERCOREMODULE_API void SetUseDefaultSync(bool);

        RENDERCOREMODULE_API [[nodiscard]] std::shared_ptr<Object> GetObjectByID(std::uint32_t);

        RENDERCOREMODULE_API [[nodiscard]] std::vector<VkImageView> GetOffscreenImages();
        RENDERCOREMODULE_API void                                   SaveOffscreenFrameToImage(strzilla::string_view);

        RENDERCOREMODULE_API [[nodiscard]] std::vector<std::shared_ptr<Texture>> LoadImages(std::vector<strzilla::string_view> &&);

        RENDERCOREMODULE_API inline void SetOnInitializeCallback(std::function<void()> &&Callback)
        {
            g_OnInitializeCallback = std::move(Callback);
        }

        RENDERCOREMODULE_API inline void SetOnRefreshCallback(std::function<void()> &&Callback)
        {
            g_OnRefreshCallback = std::move(Callback);
        }

        RENDERCOREMODULE_API inline void SetOnDrawCallback(std::function<void()> &&Callback)
        {
            g_OnDrawCallback = std::move(Callback);
        }

        RENDERCOREMODULE_API inline void SetOnShutdownCallback(std::function<void()> &&Callback)
        {
            g_OnShutdownCallback = std::move(Callback);
        }

        RENDERCOREMODULE_API [[nodiscard]] inline std::mutex &GetMutex()
        {
            return g_RendererMutex;
        }

        RENDERCOREMODULE_API inline void DispatchToMainThread(std::function<void()> &&Functor)
        {
            g_MainThreadDispatchQueue.emplace(std::move(Functor));
            glfwPostEmptyEvent();
        }

        RENDERCOREMODULE_API inline void DispatchToNextTick(std::function<void()> &&Functor)
        {
            g_NextTickDispatchQueue.emplace(std::move(Functor));
        }

        RENDERCOREMODULE_API [[nodiscard]] inline std::queue<std::function<void()>> &GetMainThreadDispatchQueue()
        {
            return g_MainThreadDispatchQueue;
        }

        RENDERCOREMODULE_API [[nodiscard]] inline bool IsInitialized()
        {
            return HasAnyFlag(g_StateFlags, RendererStateFlags::INITIALIZED | RendererStateFlags::PENDING_RESOURCES_CREATION);
        }

        RENDERCOREMODULE_API [[nodiscard]] inline bool IsReady()
        {
            return GetSwapChain() != VK_NULL_HANDLE;
        }

        RENDERCOREMODULE_API inline void AddStateFlag(RendererStateFlags const Flag)
        {
            AddFlags(g_StateFlags, Flag);
        }

        RENDERCOREMODULE_API inline void RemoveStateFlag(RendererStateFlags const Flag)
        {
            RemoveFlags(g_StateFlags, Flag);
        }

        RENDERCOREMODULE_API [[nodiscard]] inline bool HasStateFlag(RendererStateFlags const Flag)
        {
            return HasFlag(g_StateFlags, Flag);
        }

        RENDERCOREMODULE_API [[nodiscard]] inline RendererStateFlags GetStateFlags()
        {
            return g_StateFlags;
        }

        RENDERCOREMODULE_API inline void RequestLoadObject(strzilla::string_view const ObjectPath)
        {
            g_ModelsToLoad.emplace_back(ObjectPath);
            AddFlags(g_ObjectsManagementStateFlags, RendererObjectsManagementStateFlags::PENDING_LOAD);
        }

        RENDERCOREMODULE_API inline void RequestUnloadObjects(std::vector<std::uint32_t> const &ObjectIDs)
        {
            g_ModelsToUnload.insert(std::end(g_ModelsToUnload), std::begin(ObjectIDs), std::end(ObjectIDs));
            AddFlags(g_ObjectsManagementStateFlags, RendererObjectsManagementStateFlags::PENDING_UNLOAD);
        }

        RENDERCOREMODULE_API inline void RequestClearScene()
        {
            AddFlags(g_ObjectsManagementStateFlags, RendererObjectsManagementStateFlags::PENDING_CLEAR);
        }

        RENDERCOREMODULE_API inline void RequestUpdateResources()
        {
            AddFlags(g_StateFlags, RendererStateFlags::PENDING_RESOURCES_DESTRUCTION);
        }

        RENDERCOREMODULE_API [[nodiscard]] inline float const &GetFrameTime()
        {
            return g_FrameTime;
        }

        RENDERCOREMODULE_API inline void SetFPSLimit(float const MaxFPS)
        {
            if (MaxFPS > 0.F)
            {
                g_FrameRateCap = 1.F / MaxFPS;
            }
        }

        RENDERCOREMODULE_API [[nodiscard]] inline float const &GetFPSLimit()
        {
            return g_FrameRateCap;
        }

        RENDERCOREMODULE_API [[nodiscard]] inline bool const &GetVSync()
        {
            return g_UseVSync;
        }

        RENDERCOREMODULE_API [[nodiscard]] inline bool const &GetRenderOffscreen()
        {
            return g_RenderOffscreen;
        }

        RENDERCOREMODULE_API [[nodiscard]] inline bool const &GetUseDefaultSync()
        {
            return g_UseDefaultSync;
        }

        RENDERCOREMODULE_API [[nodiscard]] inline std::uint32_t const &GetImageIndex()
        {
            return g_ImageIndex;
        }

        RENDERCOREMODULE_API [[nodiscard]] inline std::uint8_t GetFrameIndex()
        {
            return static_cast<std::uint8_t>(g_ImageIndex);
        }
    } // namespace Renderer
} // namespace RenderCore
