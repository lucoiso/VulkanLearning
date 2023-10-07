// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#pragma once

#include <vk_mem_alloc.h>

export module RenderCore.Management.BufferManagement;

import <string_view>;
import <vector>;

namespace RenderCore
{
    export void CreateMemoryAllocator();
    export void CreateSwapChain();
    export void CreateFrameBuffers();
    export void CreateDepthResources();

    export std::uint32_t LoadObject(std::string_view, std::string_view);
    export void UnLoadObject(std::uint32_t);

    export void DestroyBufferResources(bool);
    export void ReleaseBufferResources();

    export [[nodiscard]] VmaAllocator const& GetAllocator();
    export [[nodiscard]] VkSwapchainKHR const& GetSwapChain();
    export [[nodiscard]] VkExtent2D const& GetSwapChainExtent();
    export [[nodiscard]] std::vector<VkFramebuffer> const& GetFrameBuffers();
    export [[nodiscard]] VkBuffer GetVertexBuffer(std::uint32_t);
    export [[nodiscard]] VkBuffer GetIndexBuffer(std::uint32_t);
    export [[nodiscard]] std::uint32_t GetIndicesCount(std::uint32_t);
    export [[nodiscard]] std::vector<struct TextureData> GetAllocatedTextures();
}// namespace RenderCore