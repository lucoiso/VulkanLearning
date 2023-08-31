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

        void CreateSwapChain(const VkSurfaceFormatKHR& PreferredFormat, const VkPresentModeKHR& PreferredMode, const VkExtent2D& PreferredExtent, const VkSurfaceCapabilitiesKHR& Capabilities);
        void CreateFrameBuffers(const VkRenderPass& RenderPass, const VkExtent2D& Extent);
        void CreateVertexBuffers();

        void Shutdown();

        bool IsInitialized() const;
        [[nodiscard]] const VkSwapchainKHR& GetSwapChain() const;
        [[nodiscard]] const std::vector<VkImage>& GetSwapChainImages() const;
        [[nodiscard]] const std::vector<VkImageView>& GetSwapChainImageViews() const;
        [[nodiscard]] const std::vector<VkFramebuffer>& GetFrameBuffers() const;
        [[nodiscard]] const std::vector<VkBuffer>& GetVertexBuffers() const;

    private:
        void CreateSwapChainImageViews(const VkFormat& ImageFormat);
        void DestroyResources();

        const VkDevice& m_Device;
        const VkSurfaceKHR& m_Surface;
        std::vector<std::uint32_t> m_QueueFamilyIndices;

        VkSwapchainKHR m_SwapChain;
        std::vector<VkImage> m_SwapChainImages;
        std::vector<VkImageView> m_SwapChainImageViews;
        std::vector<VkFramebuffer> m_FrameBuffers;
        std::vector<VkBuffer> m_VertexBuffers;
    };
}
