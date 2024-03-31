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

    std::vector<Object> AllocateScene(std::string_view);

    std::vector<Object> PrepareSceneAllocationResources(std::vector<ObjectData> &);

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

    [[nodiscard]] VkFormat const &GetDepthFormat();

    [[nodiscard]] VkSampler const &GetSampler();

    [[nodiscard]] VkBuffer GetVertexBuffer(std::uint32_t);

    [[nodiscard]] VkBuffer GetIndexBuffer(std::uint32_t);

    [[nodiscard]] std::uint32_t GetIndicesCount(std::uint32_t);

    [[nodiscard]] void *GetSceneUniformData();

    [[nodiscard]] VkDescriptorBufferInfo const &GetSceneUniformDescriptor();

    [[nodiscard]] void *GetModelUniformData(std::uint32_t);

    [[nodiscard]] bool ContainsObject(std::uint32_t);

    [[nodiscard]] std::vector<ObjectData> const &GetAllocatedObjects();

    [[nodiscard]] std::uint32_t GetNumAllocations();

    [[nodiscard]] std::uint32_t GetClampedNumAllocations();

    [[nodiscard]] VmaAllocator const &GetAllocator();

    void UpdateSceneUniformBuffers(Camera const &, Illumination const &);

    void UpdateModelUniformBuffers(std::shared_ptr<Object> const &);

    void SaveImageToFile(VkImage const &, std::string_view);
} // namespace RenderCore
