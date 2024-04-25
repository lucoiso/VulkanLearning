// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <GLFW/glfw3.h>
#include <Volk/volk.h>
#include <optional>
#include <vector>

export module RenderCore.Runtime.SwapChain;

import RenderCore.Types.Allocation;
import RenderCore.Types.SurfaceProperties;

namespace RenderCore
{
    export void CreateVulkanSurface(GLFWwindow *);
    export void CreateSwapChain(SurfaceProperties const &, VkSurfaceCapabilitiesKHR const &);

    export [[nodiscard]] VkSurfaceKHR const &                GetSurface();
    export [[nodiscard]] VkSwapchainKHR const &              GetSwapChain();
    export [[nodiscard]] VkExtent2D const &                  GetSwapChainExtent();
    export [[nodiscard]] VkFormat const &                    GetSwapChainImageFormat();
    export [[nodiscard]] std::vector<ImageAllocation> const &GetSwapChainImages();
    export [[nodiscard]] SurfaceProperties const &           GetCachedSurfaceProperties();

    export void RequestSwapChainImage(std::optional<std::int32_t> &);
    export void PresentFrame(std::uint32_t);
    export void ReleaseSwapChainResources();

    void        CreateSwapChainImageViews(std::vector<ImageAllocation> &);
    export void DestroySwapChainImages();
} // namespace RenderCore
