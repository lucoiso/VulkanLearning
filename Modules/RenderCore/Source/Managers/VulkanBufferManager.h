// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "RenderCoreModule.h"
#include "Types/VulkanVertex.h"
#include "Types/VulkanUniformBufferObject.h"
#include "Types/TextureData.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <string_view>
#include <array>

struct GLFWwindow;

namespace RenderCore
{
    class VulkanBufferManager
    {
        class Impl;

    public:
        VulkanBufferManager(const VulkanBufferManager&) = delete;
        VulkanBufferManager& operator=(const VulkanBufferManager&) = delete;

        VulkanBufferManager();
        ~VulkanBufferManager();

        void CreateMemoryAllocator();
        void CreateSwapChain(const bool bRecreate);
        void CreateFrameBuffers(const VkRenderPass& RenderPass);
        void CreateDepthResources();

        std::uint64_t LoadObject(const std::string_view Path);
        void LoadTexture(const std::string_view Path, const std::uint64_t ObjectID);

        void UpdateUniformBuffers();

        void DestroyResources(const bool bClearScene);
        void Shutdown();

        bool IsInitialized() const;
        [[nodiscard]] const VkSwapchainKHR& GetSwapChain() const;
        [[nodiscard]] const std::vector<VkImage> GetSwapChainImages() const;
        [[nodiscard]] const std::vector<VkFramebuffer> &GetFrameBuffers() const;
        [[nodiscard]] const std::vector<VkBuffer> GetVertexBuffers(const std::uint64_t ObjectID = 0u) const;
        [[nodiscard]] const std::vector<VkBuffer> GetIndexBuffers(const std::uint64_t ObjectID = 0u) const;
        [[nodiscard]] const std::vector<VkBuffer> GetUniformBuffers(const std::uint64_t ObjectID = 0u) const;
        [[nodiscard]] const std::vector<Vertex> GetVertices(const std::uint64_t ObjectID = 0u) const;
        [[nodiscard]] const std::vector<std::uint32_t> GetIndices(const std::uint64_t ObjectID = 0u) const;
        [[nodiscard]] const std::uint32_t GetIndicesCount(const std::uint64_t ObjectID = 0u) const;
        const bool GetObjectTexture(const std::uint64_t ObjectID, std::vector<VulkanTextureData> &TextureData) const;

    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
