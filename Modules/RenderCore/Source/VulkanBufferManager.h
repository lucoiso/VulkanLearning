// Copyright Notice: [...]

#pragma once

#include "RenderCoreModule.h"
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

        void InitializeSwapChain(const VkSurfaceFormatKHR& PreferredFormat, const VkPresentModeKHR& PreferredMode, const VkExtent2D& PreferredExtent, const VkSurfaceCapabilitiesKHR& Capabilities);
        void RefreshSwapChain(const VkSurfaceFormatKHR& PreferredFormat, const VkPresentModeKHR& PreferredMode, const VkExtent2D& PreferredExtent, const VkSurfaceCapabilitiesKHR& Capabilities);

        void InitializeFrameBuffers(const VkRenderPass& RenderPass, const VkExtent2D& Extent);
        void RefreshFrameBuffers(const VkRenderPass& RenderPass, const VkExtent2D& Extent);

        void InitializeVertexBuffers();
        void RefreshVertexBuffers();

        void InitializeCommandPool(const std::uint32_t GraphicsFamilyQueueIndex);
        void RefreshCommandPool(const std::uint32_t GraphicsFamilyQueueIndex);

        void Shutdown();

        bool IsInitialized() const;
        [[nodiscard]] const VkSwapchainKHR& GetSwapChain() const;
        [[nodiscard]] const std::vector<VkImage>& GetSwapChainImages() const;
        [[nodiscard]] const std::vector<VkImageView>& GetSwapChainImageViews() const;
        [[nodiscard]] const std::vector<VkFramebuffer>& GetFrameBuffers() const;
        [[nodiscard]] const std::vector<VkBuffer>& GetVertexBuffers() const;
        [[nodiscard]] const VkCommandPool& GetCommandPool() const;
        [[nodiscard]] const std::vector<VkCommandBuffer>& GetCommandBuffers() const;

    private:
        void CreateSwapChain(const VkSurfaceFormatKHR& PreferredFormat, const VkPresentModeKHR& PreferredMode, const VkExtent2D& PreferredExtent, const VkSurfaceCapabilitiesKHR& Capabilities);
        void CreateSwapChainImageViews(const VkFormat& ImageFormat);
        void CreateFrameBuffers(const VkRenderPass& RenderPass, const VkExtent2D& Extent);
        void CreateVertexBuffers();
        void CreateCommandPool(const std::uint32_t GraphicsFamilyQueueIndex);
        void DestroyResources(const bool bDestroyImages = true);

        const VkDevice& m_Device;
        const VkSurfaceKHR& m_Surface;
        const std::vector<std::uint32_t>& m_QueueFamilyIndices;

        VkSwapchainKHR m_SwapChain;
        std::vector<VkImage> m_SwapChainImages;
        std::vector<VkImageView> m_SwapChainImageViews;
        std::vector<VkFramebuffer> m_FrameBuffers;
        std::vector<VkBuffer> m_VertexBuffers;
        VkCommandPool m_CommandPool;
        std::vector<VkCommandBuffer> m_CommandBuffers;
    };
}
