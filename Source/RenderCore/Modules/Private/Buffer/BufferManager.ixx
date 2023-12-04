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
export import RenderCore.Types.AllocationTypes;

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
        void CreateMemoryAllocator();
        void CreateSwapChain();
        void CreateFrameBuffers(VkRenderPass const&);
        void CreateDepthResources();

        std::vector<Object> AllocateScene(std::string_view const&);
        void ReleaseScene(std::vector<std::uint32_t> const&);

        void DestroyBufferResources(bool);
        void ReleaseBufferResources();

        [[nodiscard]] VkSurfaceKHR& GetSurface();
        [[nodiscard]] VkSwapchainKHR const& GetSwapChain();
        [[nodiscard]] VkExtent2D const& GetSwapChainExtent();
        [[nodiscard]] std::vector<VkFramebuffer> const& GetFrameBuffers();
        [[nodiscard]] VkBuffer GetVertexBuffer(std::uint32_t);
        [[nodiscard]] VkBuffer GetIndexBuffer(std::uint32_t);
        [[nodiscard]] std::uint32_t GetIndicesCount(std::uint32_t);
        [[nodiscard]] void* GetUniformData(std::uint32_t);
        [[nodiscard]] bool ContainsObject(std::uint32_t) const;
        [[nodiscard]] std::vector<RenderCore::MeshBufferData> GetAllocatedObjects();
        [[nodiscard]] std::uint32_t GetNumAllocations() const;
        [[nodiscard]] std::uint32_t GetClampedNumAllocations() const;

        [[nodiscard]] VmaAllocator const& GetAllocator();
        void UpdateUniformBuffers(std::shared_ptr<Object> const&, VkExtent2D const&);
    };
}// namespace RenderCore