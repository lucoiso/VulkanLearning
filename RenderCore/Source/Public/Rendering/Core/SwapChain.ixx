// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Runtime.SwapChain;

import RenderCore.Utils.Constants;
import RenderCore.Types.Allocation;
import RenderCore.Types.SurfaceProperties;

namespace RenderCore
{
    RENDERCOREMODULE_API SurfaceProperties                         g_CachedProperties {};
    RENDERCOREMODULE_API VkSurfaceKHR                              g_Surface { VK_NULL_HANDLE };
    RENDERCOREMODULE_API VkSwapchainKHR                            g_SwapChain { VK_NULL_HANDLE };
    RENDERCOREMODULE_API VkSwapchainKHR                            g_OldSwapChain { VK_NULL_HANDLE };
    RENDERCOREMODULE_API std::array<ImageAllocation, g_ImageCount> g_SwapChainImages {};
}

namespace RenderCore
{
    export void CreateVulkanSurface(GLFWwindow *);
    export void CreateSwapChain(SurfaceProperties const &, VkSurfaceCapabilitiesKHR const &);

    export bool RequestSwapChainImage(std::uint32_t &);
    export void PresentFrame(std::uint32_t);
    export void ReleaseSwapChainResources();

    void        CreateSwapChainImageViews(std::array<ImageAllocation, g_ImageCount> &);
    export void DestroySwapChainImages();

    export RENDERCOREMODULE_API [[nodiscard]] inline VkSurfaceKHR const &GetSurface()
    {
        return g_Surface;
    }

    export RENDERCOREMODULE_API [[nodiscard]] inline VkSwapchainKHR const &GetSwapChain()
    {
        return g_SwapChain;
    }

    export RENDERCOREMODULE_API [[nodiscard]] inline VkExtent2D const &GetSwapChainExtent()
    {
        return g_SwapChainImages.at(0U).Extent;
    }

    export RENDERCOREMODULE_API [[nodiscard]] inline VkFormat const &GetSwapChainImageFormat()
    {
        return g_SwapChainImages.at(0U).Format;
    }

    export RENDERCOREMODULE_API [[nodiscard]] inline std::array<ImageAllocation, g_ImageCount> const &GetSwapChainImages()
    {
        return g_SwapChainImages;
    }

    export RENDERCOREMODULE_API [[nodiscard]] inline SurfaceProperties const &GetCachedSurfaceProperties()
    {
        return g_CachedProperties;
    }
} // namespace RenderCore
