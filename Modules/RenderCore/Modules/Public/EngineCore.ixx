// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <RenderCoreModule.h>

export module RenderCore.EngineCore;

export import <cstdint>;
export import <vector>;
export import <optional>;
export import <string_view>;

export import RenderCore.Types.Object;

namespace RenderCore
{
    export class RENDERCOREMODULE_API EngineCore
    {
        enum class EngineCoreStateFlags : std::uint8_t
        {
            NONE                             = 0,
            INITIALIZED                      = 1 << 0,
            PENDING_DEVICE_PROPERTIES_UPDATE = 1 << 1,
            PENDING_RESOURCES_DESTRUCTION    = 1 << 2,
            PENDING_RESOURCES_CREATION       = 1 << 3,
            PENDING_PIPELINE_REFRESH         = 1 << 4,
        };

        EngineCoreStateFlags m_StateFlags {EngineCoreStateFlags::NONE};
        std::vector<Object> m_Objects {};

        friend class Window;

        void DrawFrame(GLFWwindow*);
        std::optional<std::int32_t> TryRequestDrawImage();

        void Tick(float);

        bool InitializeEngine(GLFWwindow*);
        void ShutdownEngine();

        EngineCore()  = default;
        ~EngineCore() = default;

    public:
        static EngineCore& Get();

        [[nodiscard]] bool IsEngineInitialized() const;

        [[nodiscard]] std::vector<std::uint32_t> LoadScene(std::string_view, std::string_view);
        void UnloadScene(std::vector<std::uint32_t> const&);

        [[nodiscard]] std::vector<std::uint32_t> LoadScene(std::string_view);
        void UnloadAllScenes();

        [[nodiscard]] std::vector<Object> const& GetObjects() const;
    };
}// namespace RenderCore