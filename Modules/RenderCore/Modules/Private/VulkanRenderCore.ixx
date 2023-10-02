// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <GLFW/glfw3.h>
#include <optional>
#include <string_view>
#include <vector>
#include <volk.h>

export module RenderCore.VulkanRenderCore;

namespace RenderCore
{
    export class VulkanRenderCore final
    {
        enum class VulkanRenderCoreStateFlags : std::uint8_t
        {
            NONE                             = 0,
            INITIALIZED                      = 1 << 0,
            PENDING_DEVICE_PROPERTIES_UPDATE = 1 << 1,
            PENDING_RESOURCES_DESTRUCTION    = 1 << 2,
            PENDING_RESOURCES_CREATION       = 1 << 3,
            PENDING_PIPELINE_REFRESH         = 1 << 4,
        };

    public:
        VulkanRenderCore();

        VulkanRenderCore(VulkanRenderCore const&)            = delete;
        VulkanRenderCore& operator=(VulkanRenderCore const&) = delete;

        ~VulkanRenderCore();

        static VulkanRenderCore& Get();

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
        mutable VulkanRenderCoreStateFlags m_StateFlags;
        std::uint64_t m_ObjectID;

#ifdef _DEBUG
        VkDebugUtilsMessengerEXT m_DebugMessenger;
#endif
    };
}// namespace RenderCore