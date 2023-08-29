// Copyright Notice: [...]

#include "VulkanBufferManager.h"
#include <boost/log/trivial.hpp>

using namespace RenderCore;

VulkanBufferManager::VulkanBufferManager(const VkDevice& Device, const VkSurfaceKHR& Surface, const std::vector<std::uint32_t>& QueueFamilyIndices)
    : m_Device(Device)
    , m_Surface(Surface)
    , m_QueueFamilyIndices(QueueFamilyIndices)
    , m_SwapChain(VK_NULL_HANDLE)
    , m_SwapChainImages({})
    , m_SwapChainImageViews({})
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan buffer manager";
}

VulkanBufferManager::~VulkanBufferManager()
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing vulkan buffer manager";
    Shutdown();
}

void VulkanBufferManager::InitializeSwapChain(const VkSurfaceFormatKHR& PreferredFormat, const VkPresentModeKHR& PreferredMode, const VkExtent2D& PreferredExtent, const VkSurfaceCapabilitiesKHR& Capabilities)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Initializing Vulkan swap chain";
    CreateSwapChain(PreferredFormat, PreferredMode, PreferredExtent, Capabilities);
}

void VulkanBufferManager::RefreshSwapChain(const VkSurfaceFormatKHR& PreferredFormat, const VkPresentModeKHR& PreferredMode, const VkExtent2D& PreferredExtent, const VkSurfaceCapabilitiesKHR& Capabilities)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Refreshing Vulkan swap chain";
    CreateSwapChain(PreferredFormat, PreferredMode, PreferredExtent, Capabilities);
}

void VulkanBufferManager::CreateSwapChain(const VkSurfaceFormatKHR& PreferredFormat, const VkPresentModeKHR& PreferredMode, const VkExtent2D& PreferredExtent, const VkSurfaceCapabilitiesKHR& Capabilities)
{
    if (IsInitialized())
    {
        ResetSwapChain();
    }

    if (m_Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan logical device is invalid.");
    }

    if (m_Surface == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan surface is invalid.");
    }

    const VkSwapchainCreateInfoKHR SwapChainCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = m_Surface,
        .minImageCount = Capabilities.minImageCount,
        .imageFormat = PreferredFormat.format,
        .imageColorSpace = PreferredFormat.colorSpace,
        .imageExtent = PreferredExtent,
        .imageArrayLayers = 1u,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = m_QueueFamilyIndices.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = static_cast<std::uint32_t>(m_QueueFamilyIndices.size()),
        .pQueueFamilyIndices = m_QueueFamilyIndices.data(),
        .preTransform = Capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = PreferredMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    if (vkCreateSwapchainKHR(m_Device, &SwapChainCreateInfo, nullptr, &m_SwapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan swap chain.");
    }

    std::uint32_t Count = 0u;
    if (vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &Count, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to get Vulkan swap chain images count.");
    }

    m_SwapChainImages.resize(Count, VK_NULL_HANDLE);
    if (vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &Count, m_SwapChainImages.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to get Vulkan swap chain images.");
    }

    CreateSwapChainImageViews(PreferredFormat.format);
}

void VulkanBufferManager::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down Vulkan buffer manager";
    ResetSwapChain(false);
}

bool VulkanBufferManager::IsInitialized() const
{
    return m_Device     != VK_NULL_HANDLE
        && m_Surface    != VK_NULL_HANDLE
        && m_SwapChain  != VK_NULL_HANDLE;
}

const VkSwapchainKHR& VulkanBufferManager::GetSwapChain() const
{
    return m_SwapChain;
}

const std::vector<VkImage>& VulkanBufferManager::GetSwapChainImages() const
{
    return m_SwapChainImages;
}

const std::vector<VkImageView>& VulkanBufferManager::GetSwapChainImageViews() const
{
    return m_SwapChainImageViews;
}

void VulkanBufferManager::CreateSwapChainImageViews(const VkFormat& ImageFormat)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan image views";

    if (m_Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan logical device is invalid.");
    }

    m_SwapChainImageViews.resize(m_SwapChainImages.size(), VK_NULL_HANDLE);
    for (auto ImageIter = m_SwapChainImages.begin(); ImageIter != m_SwapChainImages.end(); ++ImageIter)
    {
        const VkImageViewCreateInfo ImageViewCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = *ImageIter,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = ImageFormat,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0u,
                .levelCount = 1u,
                .baseArrayLayer = 0u,
                .layerCount = 1u
            }
        };

        const std::int32_t Index = std::distance(m_SwapChainImages.begin(), ImageIter);
        if (vkCreateImageView(m_Device, &ImageViewCreateInfo, nullptr, &m_SwapChainImageViews[Index]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan image view.");
        }
    }
}

void VulkanBufferManager::ResetSwapChain(const bool bDestroyImages)
{
    if (m_Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan logical device is invalid.");
    }

    if (m_SwapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
        m_SwapChain = VK_NULL_HANDLE;
    }

    if (bDestroyImages)
    {
        for (const VkImage& ImageIter : m_SwapChainImages)
        {
            if (ImageIter != VK_NULL_HANDLE)
            {
                vkDestroyImage(m_Device, ImageIter, nullptr);
            }
        }
    }
    m_SwapChainImages.clear();

    for (const VkImageView& ImageViewIter : m_SwapChainImageViews)
    {
        if (ImageViewIter != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_Device, ImageViewIter, nullptr);
        }
    }
    m_SwapChainImageViews.clear();
}
