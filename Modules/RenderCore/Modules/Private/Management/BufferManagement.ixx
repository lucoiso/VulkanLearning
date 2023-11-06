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

export import RenderCore.Types.Coroutine;
export import RenderCore.Types.ObjectData;
export import RenderCore.Types.TextureData;
export import RenderCore.Types.UniformBufferObject;

namespace RenderCore
{
    export void CreateVulkanSurface(GLFWwindow*);
    export void CreateMemoryAllocator();
    export void CreateSwapChain();
    export void CreateFrameBuffers();
    export void CreateDepthResources();

    export AsyncOperation<std::vector<std::uint32_t>> AllocateScene(std::string_view, std::string_view);
    export AsyncTask ReleaseScene(std::vector<std::uint32_t> const&);

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
    export [[nodiscard]] std::vector<TextureData> GetAllocatedTextures();
    export [[nodiscard]] std::vector<ObjectData> GetAllocatedObjects();

    export void UpdateUniformBuffers();
}// namespace RenderCore