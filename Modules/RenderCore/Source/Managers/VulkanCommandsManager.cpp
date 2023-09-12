// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "Managers/VulkanCommandsManager.h"
#include "Utils/RenderCoreHelpers.h"
#include <boost/log/trivial.hpp>

using namespace RenderCore;

VulkanCommandsManager::VulkanCommandsManager(const VkDevice &Device)
	: m_Device(Device)
	, m_CommandPool(VK_NULL_HANDLE)
	, m_CommandBuffer(VK_NULL_HANDLE)
	, m_ImageAvailableSemaphore(VK_NULL_HANDLE)
	, m_RenderFinishedSemaphore(VK_NULL_HANDLE)
	, m_Fence(VK_NULL_HANDLE)
	, m_CurrentFrameIndex(0u)
	, m_SynchronizationObjectsCreated(false)
	, m_GraphicsProcessingFamilyQueueIndex(std::optional<std::uint32_t>())
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
	Shutdown({});
}

void VulkanCommandsManager::Shutdown(const std::vector<VkQueue> &PendingQueues)
{
	if (!IsInitialized())
	{
		return;
	}

	BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down Vulkan commands manager";

	WaitAndResetFences();

	if (m_CommandBuffer != VK_NULL_HANDLE)
	{
		vkFreeCommandBuffers(m_Device, m_CommandPool, 1u, &m_CommandBuffer);
		m_CommandBuffer = VK_NULL_HANDLE;
	}

	if (m_CommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
		m_CommandPool = VK_NULL_HANDLE;	
	}

	DestroySynchronizationObjects();
}

void VulkanCommandsManager::SetGraphicsProcessingFamilyQueueIndex(const std::uint32_t FamilyQueueIndex)
{
	m_GraphicsProcessingFamilyQueueIndex = FamilyQueueIndex;
}

VkCommandPool VulkanCommandsManager::CreateCommandPool(const std::uint32_t FamilyQueueIndex)
{
	if (m_Device == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan logical device is invalid.");
	}

	const VkCommandPoolCreateInfo CommandPoolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
		.queueFamilyIndex = FamilyQueueIndex};

	VkCommandPool Output = VK_NULL_HANDLE;
	RENDERCORE_CHECK_VULKAN_RESULT(vkCreateCommandPool(m_Device, &CommandPoolCreateInfo, nullptr, &Output));

	return Output;
}

void VulkanCommandsManager::CreateSynchronizationObjects()
{
	if (m_SynchronizationObjectsCreated)
	{
		return;
	}

	BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan synchronization objects";

	if (m_Device == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan logical device is invalid.");
	}

	const VkSemaphoreCreateInfo SemaphoreCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

	const VkFenceCreateInfo FenceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT};

	m_ImageAvailableSemaphore = VK_NULL_HANDLE;
	m_RenderFinishedSemaphore = VK_NULL_HANDLE;
	m_Fence = VK_NULL_HANDLE;

	RENDERCORE_CHECK_VULKAN_RESULT(vkCreateSemaphore(m_Device, &SemaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphore));
	RENDERCORE_CHECK_VULKAN_RESULT(vkCreateSemaphore(m_Device, &SemaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphore));
	RENDERCORE_CHECK_VULKAN_RESULT(vkCreateFence(m_Device, &FenceCreateInfo, nullptr, &m_Fence));

	m_SynchronizationObjectsCreated = true;
}

void VulkanCommandsManager::DestroySynchronizationObjects()
{
	if (!m_SynchronizationObjectsCreated)
	{
		return;
	}

	BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying Vulkan synchronization objects";

	vkDeviceWaitIdle(m_Device);

	if (m_CommandBuffer != VK_NULL_HANDLE)
	{
		vkFreeCommandBuffers(m_Device, m_CommandPool, 1u, &m_CommandBuffer);
		m_CommandBuffer = VK_NULL_HANDLE;
	}

	if (m_CommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
		m_CommandPool = VK_NULL_HANDLE;
	}

	if (m_ImageAvailableSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(m_Device, m_ImageAvailableSemaphore, nullptr);
		m_ImageAvailableSemaphore = VK_NULL_HANDLE;
	}

	if (m_RenderFinishedSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(m_Device, m_RenderFinishedSemaphore, nullptr);
		m_RenderFinishedSemaphore = VK_NULL_HANDLE;
	}

	if (m_Fence != VK_NULL_HANDLE)
	{
		vkDestroyFence(m_Device, m_Fence, nullptr);
		m_Fence = VK_NULL_HANDLE;
	}

	m_SynchronizationObjectsCreated = false;
}

std::int32_t VulkanCommandsManager::DrawFrame(const VkSwapchainKHR &SwapChain)
{
	if (m_Device == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan logical device is invalid.");
	}

	if (!m_SynchronizationObjectsCreated)
	{
		return -1;
	}

	if (SwapChain == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan swap chain is invalid.");
	}

	WaitAndResetFences();

	if (m_ImageAvailableSemaphore == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan semaphore: Image Available is invalid.");
	}

	if (m_Fence == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan fence is invalid.");
	}

	std::uint32_t Output = 0u;
	if (const VkResult OperationResult = vkAcquireNextImageKHR(m_Device, SwapChain, Timeout, m_ImageAvailableSemaphore, m_Fence, &Output); OperationResult != VK_SUCCESS)
	{
		if (OperationResult == VK_ERROR_OUT_OF_DATE_KHR || OperationResult == VK_SUBOPTIMAL_KHR)
		{
			if (OperationResult == VK_ERROR_OUT_OF_DATE_KHR)
			{
				BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Failed to acquire next image: Vulkan swap chain is outdated";
			}
			else if (OperationResult == VK_SUBOPTIMAL_KHR)
			{
				BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Failed to acquire next image: Vulkan swap chain is suboptimal";
				WaitAndResetFences();
			}

			return -1;
		}
		else if (OperationResult != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to acquire Vulkan swap chain image.");
		}
	}

	return static_cast<std::int32_t>(Output);
}

void VulkanCommandsManager::RecordCommandBuffers(const BufferRecordParameters &Parameters)
{
	if (Parameters.Pipeline == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan graphics pipeline is invalid");
	}
	
	AllocateCommandBuffer();

	const VkCommandBufferBeginInfo CommandBufferBeginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

	RENDERCORE_CHECK_VULKAN_RESULT(vkBeginCommandBuffer(m_CommandBuffer, &CommandBufferBeginInfo));

	const std::array<VkClearValue, 2> ClearValues{
		VkClearValue{.color = {0.0f, 0.0f, 0.0f, 1.0f}},
		VkClearValue{.depthStencil = {1.0f, 0u}}};

	const VkRenderPassBeginInfo RenderPassBeginInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = Parameters.RenderPass,
		.framebuffer = Parameters.FrameBuffers[Parameters.ImageIndex],
		.renderArea = {
			.offset = {0, 0},
			.extent = Parameters.Extent},
		.clearValueCount = static_cast<std::uint32_t>(ClearValues.size()),
		.pClearValues = ClearValues.data()};

	vkCmdBeginRenderPass(m_CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Parameters.Pipeline);

	vkCmdBindVertexBuffers(m_CommandBuffer, 0u, 1u, Parameters.VertexBuffers.data(), Parameters.Offsets.data());

	for (const VkBuffer &IndexBufferIter : Parameters.IndexBuffers)
	{
		vkCmdBindIndexBuffer(m_CommandBuffer, IndexBufferIter, 0u, VK_INDEX_TYPE_UINT32);
	}

	const VkViewport Viewport{
		.x = 0.f,
		.y = 0.f,
		.width = static_cast<float>(Parameters.Extent.width),
		.height = static_cast<float>(Parameters.Extent.height),
		.minDepth = 0.f,
		.maxDepth = 1.f
	};

	vkCmdSetViewport(m_CommandBuffer, 0u, 1u, &Viewport);

	const VkRect2D Scissor{
		.offset = {0, 0},
		.extent = Parameters.Extent};

	vkCmdSetScissor(m_CommandBuffer, 0u, 1u, &Scissor);

	vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Parameters.PipelineLayout, 0u, 1u, &Parameters.DescriptorSets[m_CurrentFrameIndex], 0u, nullptr);

	vkCmdDrawIndexed(m_CommandBuffer, Parameters.IndexCount, 1u, 0u, 0u, 0u);
	vkCmdEndRenderPass(m_CommandBuffer);

	RENDERCORE_CHECK_VULKAN_RESULT(vkEndCommandBuffer(m_CommandBuffer));
}

void VulkanCommandsManager::SubmitCommandBuffers(const VkQueue &Queue)
{
	if (Queue == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan graphics queue is invalid.");
	}

	WaitAndResetFences();

	constexpr VkPipelineStageFlags WaitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

	const VkSubmitInfo SubmitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1u,
		.pWaitSemaphores = &m_ImageAvailableSemaphore,
		.pWaitDstStageMask = WaitStages,
		.commandBufferCount = 1u,
		.pCommandBuffers = &m_CommandBuffer,
		.signalSemaphoreCount = 1u,
		.pSignalSemaphores = &m_RenderFinishedSemaphore};
	
	RENDERCORE_CHECK_VULKAN_RESULT(vkQueueSubmit(Queue, 1u, &SubmitInfo, m_Fence));
    RENDERCORE_CHECK_VULKAN_RESULT(vkQueueWaitIdle(Queue));

	vkFreeCommandBuffers(m_Device, m_CommandPool, 1u, &m_CommandBuffer);
	m_CommandBuffer = VK_NULL_HANDLE;
}

void VulkanCommandsManager::PresentFrame(const VkQueue &Queue, const VkSwapchainKHR &SwapChain, const std::uint32_t ImageIndice)
{
	if (Queue == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan present queue is invalid.");
	}

	const VkPresentInfoKHR PresentInfo{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1u,
		.pWaitSemaphores = &m_RenderFinishedSemaphore,
		.swapchainCount = 1u,
		.pSwapchains = &SwapChain,
		.pImageIndices = &ImageIndice,
		.pResults = nullptr};

	if (const VkResult OperationResult = vkQueuePresentKHR(Queue, &PresentInfo); OperationResult != VK_SUCCESS)
	{
		if (OperationResult != VK_ERROR_OUT_OF_DATE_KHR && OperationResult != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Vulkan operation failed with result: " + std::string(ResultToString(OperationResult)));
		}
	}

	m_CurrentFrameIndex = (m_CurrentFrameIndex + 1u) % g_MaxFramesInFlight;
}

std::uint32_t VulkanCommandsManager::GetCurrentFrameIndex() const
{
    return m_CurrentFrameIndex;
}

bool VulkanCommandsManager::IsInitialized() const
{
	return m_GraphicsProcessingFamilyQueueIndex.has_value();
}

void VulkanCommandsManager::CreateGraphicsCommandPool()
{
	m_CommandPool = CreateCommandPool(m_GraphicsProcessingFamilyQueueIndex.value());
}

void VulkanCommandsManager::AllocateCommandBuffer()
{
	if (m_CommandBuffer != VK_NULL_HANDLE)
	{
		vkFreeCommandBuffers(m_Device, m_CommandPool, 1u, &m_CommandBuffer);
		m_CommandBuffer = VK_NULL_HANDLE;
	}

	if (m_CommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
		m_CommandPool = VK_NULL_HANDLE;
	}

	CreateGraphicsCommandPool();

	const VkCommandBufferAllocateInfo CommandBufferAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = m_CommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1u};

	RENDERCORE_CHECK_VULKAN_RESULT(vkAllocateCommandBuffers(m_Device, &CommandBufferAllocateInfo, &m_CommandBuffer));
}

void VulkanCommandsManager::WaitAndResetFences()
{
	if (m_Fence == VK_NULL_HANDLE)
	{
		return;
	}

	RENDERCORE_CHECK_VULKAN_RESULT(vkWaitForFences(m_Device, 1u, &m_Fence, VK_TRUE, Timeout));
	RENDERCORE_CHECK_VULKAN_RESULT(vkResetFences(m_Device, 1u, &m_Fence));
}
