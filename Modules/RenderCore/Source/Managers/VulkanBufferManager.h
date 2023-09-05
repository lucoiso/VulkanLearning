// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include "RenderCoreModule.h"
#include "Types/VulkanVertex.h"
#include "Types/VulkanUniformBufferObject.h"
#include <vulkan/vulkan.h>
#include <vector>

struct GLFWwindow;

namespace RenderCore
{
    class VulkanBufferManager
    {
    public:
        VulkanBufferManager() = delete;
        VulkanBufferManager(const VulkanBufferManager&) = delete;
        VulkanBufferManager& operator=(const VulkanBufferManager&) = delete;

        VulkanBufferManager(const VkDevice& Device, const VkSurfaceKHR& Surface, const std::vector<std::uint32_t>& QueueFamilyIndices);
        ~VulkanBufferManager();

        void CreateSwapChain(const VkSurfaceFormatKHR& PreferredFormat, const VkPresentModeKHR& PreferredMode, const VkExtent2D& PreferredExtent, const VkSurfaceCapabilitiesKHR& Capabilities);
        void CreateFrameBuffers(const VkRenderPass& RenderPass, const VkExtent2D& Extent);
        void CreateVertexBuffers(const VkPhysicalDevice& PhysicalDevice, const VkQueue& TransferQueue, const VkCommandPool& CommandPool);
        void CreateIndexBuffers(const VkPhysicalDevice& PhysicalDevice, const VkQueue& TransferQueue, const VkCommandPool& CommandPool);
        void CreateUniformBuffers(const VkPhysicalDevice& PhysicalDevice, const VkQueue& TransferQueue, const VkCommandPool& CommandPool);
        
        void CreateBuffer(const VkPhysicalDeviceMemoryProperties& Properties, const VkDeviceSize& Size, const VkBufferUsageFlags& Usage, const VkMemoryPropertyFlags& Flags, VkBuffer& Buffer, VkDeviceMemory& BufferMemory) const;
        void CopyBuffer(const VkBuffer& Source, const VkBuffer& Destination, const VkDeviceSize& Size, const VkQueue& TransferQueue, const VkCommandPool& CommandPool) const;

        void UpdateUniformBuffers(const std::uint32_t Frame, const VkExtent2D &SwapChainExtent);

        void Shutdown();

        bool IsInitialized() const;
        [[nodiscard]] const VkSwapchainKHR& GetSwapChain() const;
        [[nodiscard]] const std::vector<VkImage>& GetSwapChainImages() const;
        [[nodiscard]] const std::vector<VkImageView>& GetSwapChainImageViews() const;
        [[nodiscard]] const std::vector<VkFramebuffer>& GetFrameBuffers() const;
        [[nodiscard]] const std::vector<VkBuffer>& GetVertexBuffers() const;
        [[nodiscard]] const std::vector<VkDeviceMemory>& GetVertexBuffersMemory() const;
        [[nodiscard]] const std::vector<VkBuffer>& GetIndexBuffers() const;
        [[nodiscard]] const std::vector<VkDeviceMemory>& GetIndexBuffersMemory() const;
        [[nodiscard]] const std::vector<VkBuffer>& GetUniformBuffers() const;
        [[nodiscard]] const std::vector<VkDeviceMemory>& GetUniformBuffersMemory() const;
        [[nodiscard]] const std::vector<void*>& GetUniformBuffersData() const;
        [[nodiscard]] const std::vector<Vertex>& GetVertices() const;
        [[nodiscard]] const std::vector<std::uint16_t>& GetIndices() const;
        [[nodiscard]] std::uint32_t GetIndexCount() const;

    private:
        void CreateSwapChainImageViews(const VkFormat& ImageFormat);
        void DestroyResources();

        std::uint32_t FindMemoryType(const VkPhysicalDeviceMemoryProperties& Properties, const std::uint32_t& TypeFilter, const VkMemoryPropertyFlags& Flags) const;

        const VkDevice& m_Device;
        const VkSurfaceKHR& m_Surface;
        const std::vector<std::uint32_t> m_QueueFamilyIndices;

        VkSwapchainKHR m_SwapChain;
        std::vector<VkImage> m_SwapChainImages;
        std::vector<VkImageView> m_SwapChainImageViews;
        std::vector<VkFramebuffer> m_FrameBuffers;
        std::vector<VkBuffer> m_VertexBuffers;
        std::vector<VkDeviceMemory> m_VertexBuffersMemory;
        std::vector<VkBuffer> m_IndexBuffers;
        std::vector<VkDeviceMemory> m_IndexBuffersMemory;
        std::vector<VkBuffer> m_UniformBuffers;
        std::vector<VkDeviceMemory> m_UniformBuffersMemory;
        std::vector<void*> m_UniformBuffersData;
        std::vector<Vertex> m_Vertices;
        std::vector<std::uint16_t> m_Indices;
    };
}
