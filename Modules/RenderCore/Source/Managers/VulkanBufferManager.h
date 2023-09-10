// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "RenderCoreModule.h"
#include "Types/VulkanVertex.h"
#include "Types/VulkanUniformBufferObject.h"
#include <vulkan/vulkan_core.h>
#include <vector>
#include <memory>

struct GLFWwindow;

namespace RenderCore
{
    class VulkanBufferManager
    {
        class Impl;

    public:
        VulkanBufferManager() = delete;
        VulkanBufferManager(const VulkanBufferManager&) = delete;
        VulkanBufferManager& operator=(const VulkanBufferManager&) = delete;

        VulkanBufferManager(const VkDevice& Device, const VkSurfaceKHR& Surface, const std::vector<std::uint32_t>& QueueFamilyIndices);
        ~VulkanBufferManager();

        void CreateMemoryAllocator(const VkInstance& Instance, const VkDevice& LogicalDevice, const VkPhysicalDevice& PhysicalDevice);
        void CreateSwapChain(const VkSurfaceFormatKHR& PreferredFormat, const VkPresentModeKHR& PreferredMode, const VkExtent2D& PreferredExtent, const VkSurfaceCapabilitiesKHR& Capabilities);
        void CreateFrameBuffers(const VkRenderPass& RenderPass, const VkExtent2D& Extent);
        void CreateVertexBuffers(const VkPhysicalDevice& PhysicalDevice, const VkQueue& TransferQueue, const VkCommandPool& CommandPool);
        void CreateIndexBuffers(const VkPhysicalDevice& PhysicalDevice, const VkQueue& TransferQueue, const VkCommandPool& CommandPool);
        void CreateUniformBuffers(const VkPhysicalDevice& PhysicalDevice, const VkQueue& TransferQueue);
        void UpdateUniformBuffers(const std::uint32_t Frame, const VkExtent2D &SwapChainExtent);

        void Shutdown();

        bool IsInitialized() const;
        [[nodiscard]] const VkSwapchainKHR& GetSwapChain() const;
        [[nodiscard]] const std::vector<VkImage>& GetSwapChainImages() const;
        [[nodiscard]] const std::vector<VkFramebuffer>& GetFrameBuffers() const;
        [[nodiscard]] const std::vector<VkBuffer>& GetVertexBuffers() const;
        [[nodiscard]] const std::vector<VkBuffer>& GetIndexBuffers() const;
        [[nodiscard]] const std::vector<VkBuffer>& GetUniformBuffers() const;
        [[nodiscard]] const std::vector<void*>& GetUniformBuffersData() const;
        [[nodiscard]] const std::vector<Vertex>& GetVertices() const;
        [[nodiscard]] const std::vector<std::uint16_t>& GetIndices() const;
        [[nodiscard]] std::uint32_t GetIndexCount() const;

    private:
        std::unique_ptr<Impl> m_Impl;
    };
}
