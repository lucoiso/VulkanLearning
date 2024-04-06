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

export namespace RenderCore
{
    void CreateVulkanSurface(GLFWwindow *);

    void CreateSwapChain(SurfaceProperties const &, VkSurfaceCapabilitiesKHR const &);

    [[nodiscard]] VkSurfaceKHR const &GetSurface();

    [[nodiscard]] VkSwapchainKHR const &GetSwapChain();

    [[nodiscard]] VkExtent2D const &GetSwapChainExtent();

    [[nodiscard]] VkFormat const &GetSwapChainImageFormat();

    [[nodiscard]] std::vector<ImageAllocation> const &GetSwapChainImages();

    [[nodiscard]] std::optional<std::int32_t> RequestSwapChainImage();

    void PresentFrame(std::uint32_t);

    void CreateSwapChainImageViews(std::vector<ImageAllocation> &);

    void ReleaseSwapChainResources();

    void DestroySwapChainImages();
} // namespace RenderCore
