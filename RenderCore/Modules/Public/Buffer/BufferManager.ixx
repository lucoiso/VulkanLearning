// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <vk_mem_alloc.h>

export module RenderCore.Management.BufferManagement;

import RenderCore.Types.Object;
import RenderCore.Types.Camera;
import RenderCore.Types.Transform;
import RenderCore.Types.AllocationTypes;
import RenderCore.Types.SurfaceProperties;
import RenderCore.Types.MeshBufferData;

namespace RenderCore
{
    export class BufferManager
    {
        VkSurfaceKHR m_Surface {VK_NULL_HANDLE};
        VmaAllocator m_Allocator {VK_NULL_HANDLE};
        VkSwapchainKHR m_SwapChain {VK_NULL_HANDLE};
        VkSwapchainKHR m_OldSwapChain {VK_NULL_HANDLE};
        VkExtent2D m_SwapChainExtent {0U, 0U};

        std::vector<ImageAllocation> m_SwapChainImages {};
        std::vector<VkFramebuffer> m_SwapChainFrameBuffers {};

        std::vector<ImageAllocation> m_ViewportImages {};
        std::vector<VkFramebuffer> m_ViewportFrameBuffers {};

        VkSampler m_Sampler {VK_NULL_HANDLE};
        ImageAllocation m_DepthImage {};
        std::unordered_map<std::uint32_t, ObjectAllocation> m_Objects {};
        std::atomic<std::uint32_t> m_ObjectIDCounter {0U};

    public:
        void CreateVulkanSurface(GLFWwindow*);
        void CreateMemoryAllocator(VkPhysicalDevice const&);
        void CreateImageSampler();
        void CreateSwapChain(SurfaceProperties const&, VkSurfaceCapabilitiesKHR const&);
        void CreateSwapChainFrameBuffers(VkRenderPass const&);
        void CreateViewportResources(SurfaceProperties const&);
        void CreateViewportFrameBuffer(VkRenderPass const&);
        [[nodiscard]] std::vector<VkFramebuffer> CreateFrameBuffers(VkRenderPass const&, std::vector<ImageAllocation> const&, bool) const;
        void CreateDepthResources(SurfaceProperties const&);

        std::vector<Object> AllocateScene(std::string_view const&);
        void ReleaseScene(std::vector<std::uint32_t> const&);

        void DestroyBufferResources(bool);
        void ReleaseBufferResources();

        [[nodiscard]] VkSurfaceKHR const& GetSurface() const;
        [[nodiscard]] VkSwapchainKHR const& GetSwapChain() const;
        [[nodiscard]] VkExtent2D const& GetSwapChainExtent() const;
        [[nodiscard]] std::vector<VkImageView> GetSwapChainImageViews() const;
        [[nodiscard]] std::vector<VkImageView> GetViewportImageViews() const;
        [[nodiscard]] std::vector<ImageAllocation> const& GetSwapChainImages() const;
        [[nodiscard]] std::vector<ImageAllocation> const& GetViewportImages() const;
        [[nodiscard]] VkSampler const& GetSampler() const;
        [[nodiscard]] std::vector<VkFramebuffer> const& GetSwapChainFrameBuffers() const;
        [[nodiscard]] std::vector<VkFramebuffer> const& GetViewportFrameBuffers() const;
        [[nodiscard]] VkBuffer GetVertexBuffer(std::uint32_t) const;
        [[nodiscard]] VkBuffer GetIndexBuffer(std::uint32_t) const;
        [[nodiscard]] std::uint32_t GetIndicesCount(std::uint32_t) const;
        [[nodiscard]] void* GetUniformData(std::uint32_t) const;
        [[nodiscard]] bool ContainsObject(std::uint32_t) const;
        [[nodiscard]] std::vector<MeshBufferData> GetAllocatedObjects() const;
        [[nodiscard]] std::uint32_t GetNumAllocations() const;
        [[nodiscard]] std::uint32_t GetClampedNumAllocations() const;

        [[nodiscard]] VmaAllocator const& GetAllocator() const;
        void UpdateUniformBuffers(std::shared_ptr<Object> const&, Camera const&, VkExtent2D const&) const;
    };
}// namespace RenderCore