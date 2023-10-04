// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <GLFW/glfw3.h>
#include <volk.h>

export module RenderCore.EngineCore;

import <array>;
import <cstdint>;
import <filesystem>;
import <optional>;
import <stdexcept>;
import <string_view>;
import <vector>;

export namespace RenderCore
{
    class EngineCore final
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

    public:
        EngineCore();

        EngineCore(EngineCore const&)            = delete;
        EngineCore& operator=(EngineCore const&) = delete;

        ~EngineCore();

        static EngineCore& Get();

        void Initialize(GLFWwindow* Window);
        void Shutdown();

        void DrawFrame(GLFWwindow* Window) const;
        bool IsInitialized() const;

        void LoadScene(std::string_view ModelPath, std::string_view TexturePath);
        void UnloadScene() const;

        [[nodiscard]] VkInstance& GetInstance();
        [[nodiscard]] VkSurfaceKHR& GetSurface();

    private:
        std::optional<std::int32_t> TryRequestDrawImage() const;

        void CreateVulkanInstance();
        void CreateVulkanSurface(GLFWwindow* Window);
        void InitializeRenderCore() const;

        static std::vector<VkPipelineShaderStageCreateInfo> CompileDefaultShaders();

        VkInstance m_Instance;
        VkSurfaceKHR m_Surface;
        mutable EngineCoreStateFlags m_StateFlags;
        std::uint64_t m_ObjectID;

#ifdef _DEBUG
        VkDebugUtilsMessengerEXT m_DebugMessenger;
#endif
    };
}// namespace RenderCore