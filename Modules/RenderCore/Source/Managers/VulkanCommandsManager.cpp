// Copyright Notice: [...]

#include "Managers/VulkanCommandsManager.h"
#include <boost/log/trivial.hpp>

using namespace RenderCore;

VulkanCommandsManager::VulkanCommandsManager(const VkDevice& Device)
    : m_Device(Device)
    , m_CommandPool(VK_NULL_HANDLE)
    , m_CommandBuffers({})
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan commands manager";
}

VulkanCommandsManager::~VulkanCommandsManager()
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing vulkan commands manager";
    Shutdown();
}

void VulkanCommandsManager::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down Vulkan commands manager";

    vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
    m_CommandPool = VK_NULL_HANDLE;

    // Automatically freed when the command pool is destroyed
    m_CommandBuffers.clear();
}

void VulkanCommandsManager::CreateCommandPool(const std::vector<VkFramebuffer>& FrameBuffers, const std::uint32_t GraphicsFamilyQueueIndex)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan command pool";

    if (m_Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Vulkan logical device is invalid.");
    }

    const VkCommandPoolCreateInfo CommandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = GraphicsFamilyQueueIndex
    };

    if (vkCreateCommandPool(m_Device, &CommandPoolCreateInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan command pool.");
    }

    const VkCommandBufferAllocateInfo CommandBufferAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_CommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<std::uint32_t>(FrameBuffers.size())
    };

    m_CommandBuffers.resize(FrameBuffers.size(), VK_NULL_HANDLE);
    if (vkAllocateCommandBuffers(m_Device, &CommandBufferAllocateInfo, m_CommandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate Vulkan command buffers.");
    }

    const VkCommandBufferBeginInfo CommandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
    };

    for (const VkCommandBuffer& CommandBufferIt : m_CommandBuffers)
    {
        if (vkBeginCommandBuffer(CommandBufferIt, &CommandBufferBeginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin Vulkan command buffer.");
        }
    }
}

bool VulkanCommandsManager::IsInitialized() const
{
    return m_CommandPool != VK_NULL_HANDLE;
}

const VkCommandPool& VulkanCommandsManager::GetCommandPool() const
{
    return m_CommandPool;
}

const std::vector<VkCommandBuffer>& VulkanCommandsManager::GetCommandBuffers() const
{
    return m_CommandBuffers;
}