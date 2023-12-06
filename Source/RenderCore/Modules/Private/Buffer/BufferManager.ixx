// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>

export module RenderCore.Management.BufferManagement;

export import <string_view>;
export import <cstdint>;
export import <vector>;
export import <atomic>;
export import <memory>;

export import RenderCore.Types.Object;
export import RenderCore.Types.Camera;
export import RenderCore.Types.AllocationTypes;
export import RenderCore.Types.SurfaceProperties;

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
        ImageAllocation m_DepthImage {};
        std::vector<VkFramebuffer> m_FrameBuffers {};
        std::unordered_map<std::uint32_t, ObjectAllocation> m_Objects {};
        std::atomic<std::uint32_t> m_ObjectIDCounter {0U};

    public:
        void CreateVulkanSurface(GLFWwindow*);
        void CreateMemoryAllocator(VkPhysicalDevice const&);
        void CreateSwapChain(SurfaceProperties const&, VkSurfaceCapabilitiesKHR const&, std::vector<std::uint32_t> const&);
        void CreateFrameBuffers(VkRenderPass const&);
        void CreateDepthResources(SurfaceProperties const&, std::pair<std::uint8_t, VkQueue> const&);

        std::vector<Object> AllocateScene(std::string_view const&, std::pair<std::uint8_t, VkQueue> const&);
        void ReleaseScene(std::vector<std::uint32_t> const&);

        void DestroyBufferResources(bool);
        void ReleaseBufferResources();

        [[nodiscard]] VkSurfaceKHR const& GetSurface() const;
        [[nodiscard]] VkSwapchainKHR const& GetSwapChain() const;
        [[nodiscard]] VkExtent2D const& GetSwapChainExtent() const;
        [[nodiscard]] std::vector<VkFramebuffer> const& GetFrameBuffers() const;
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