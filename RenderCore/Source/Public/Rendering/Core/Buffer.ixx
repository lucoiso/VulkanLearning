// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <vma/vk_mem_alloc.h>

export module RenderCore.Runtime.Buffer;

import RenderCore.Types.Object;
import RenderCore.Types.Allocation;
import RenderCore.Types.SurfaceProperties;
import RenderCore.Types.Camera;
import RenderCore.Types.Illumination;

export namespace RenderCore
{
    void CreateVulkanSurface(GLFWwindow *);

    void CreateMemoryAllocator(VkPhysicalDevice const &);

    void CreateSceneUniformBuffers();

    void CreateImageSampler();

    void CreateSwapChain(SurfaceProperties const &, VkSurfaceCapabilitiesKHR const &);

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
    void CreateViewportResources(SurfaceProperties const &);
#endif

    void CreateDepthResources(SurfaceProperties const &);

    void AllocateEmptyTexture(VkFormat);

    void AllocateScene(std::string_view);

    void ReleaseScene(std::vector<std::uint32_t> const &);

    void DestroyBufferResources(bool);

    void ReleaseBufferResources();

    [[nodiscard]] VkSurfaceKHR const &GetSurface();

    [[nodiscard]] VkSwapchainKHR const &GetSwapChain();

    [[nodiscard]] VkExtent2D const &GetSwapChainExtent();

    [[nodiscard]] VkFormat const &GetSwapChainImageFormat();

    [[nodiscard]] std::vector<ImageAllocation> const &GetSwapChainImages();

#ifdef VULKAN_RENDERER_ENABLE_IMGUI
    [[nodiscard]] std::vector<ImageAllocation> const &GetViewportImages();
#endif

    [[nodiscard]] ImageAllocation const &GetDepthImage();

    [[nodiscard]] VkSampler const &GetSampler();

    [[nodiscard]] ImageAllocation const &GetEmptyImage();

    [[nodiscard]] void *GetSceneUniformData();

    [[nodiscard]] VkDescriptorBufferInfo const &GetSceneUniformDescriptor();

    [[nodiscard]] std::vector<std::shared_ptr<Object>> const &GetAllocatedObjects();

    [[nodiscard]] std::uint32_t GetNumAllocations();

    [[nodiscard]] VmaAllocator const &GetAllocator();

    void UpdateSceneUniformBuffers(Camera const &, Illumination const &);

    void SaveImageToFile(VkImage const &, std::string_view);
} // namespace RenderCore
