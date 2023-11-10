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

export import RenderCore.Types.Object;
export import RenderCore.Types.ObjectBufferData;

namespace RenderCore
{
    export void CreateVulkanSurface(GLFWwindow*);
    export void CreateMemoryAllocator();
    export void CreateSwapChain();
    export void CreateFrameBuffers();
    export void CreateDepthResources();

    export std::vector<Object> AllocateScene(std::string_view const&);
    export void ReleaseScene(std::vector<std::uint32_t> const&);

    export void DestroyBufferResources(bool);
    export void ReleaseBufferResources();

    export [[nodiscard]] VkSurfaceKHR& GetSurface();
    export [[nodiscard]] VmaAllocator const& GetAllocator();
    export [[nodiscard]] VkSwapchainKHR const& GetSwapChain();
    export [[nodiscard]] VkExtent2D const& GetSwapChainExtent();
    export [[nodiscard]] std::vector<VkFramebuffer> const& GetFrameBuffers();
    export [[nodiscard]] VkBuffer GetVertexBuffer(std::uint32_t);
    export [[nodiscard]] VkBuffer GetIndexBuffer(std::uint32_t);
    export [[nodiscard]] std::uint32_t GetIndicesCount(std::uint32_t);
    export [[nodiscard]] void* GetUniformData(std::uint32_t);
    export [[nodiscard]] bool ContainsObject(std::uint32_t);
    export [[nodiscard]] std::vector<ObjectBufferData> GetAllocatedObjects();
    export [[nodiscard]] std::uint32_t GetNumAllocations();

    export void UpdateUniformBuffers();
}// namespace RenderCore