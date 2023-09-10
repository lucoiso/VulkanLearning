// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "Managers/VulkanCommandsManager.h"
#include "Utils/RenderCoreHelpers.h"
#include <boost/log/trivial.hpp>
#include "VulkanCommandsManager.h"

using namespace RenderCore;

VulkanCommandsManager::VulkanCommandsManager(const VkDevice &Device)
	: m_Device(Device)
	, m_GraphicsCommandPool(VK_NULL_HANDLE)
	, m_CommandBuffers({})
	, m_ImageAvailableSemaphores({})
	, m_RenderFinishedSemaphores({})
	, m_Fences({})
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

	if (!m_CommandBuffers.empty())
	{
		vkFreeCommandBuffers(m_Device, m_GraphicsCommandPool, static_cast<std::uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());
	}
	m_CommandBuffers.clear();

	if (m_GraphicsCommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(m_Device, m_GraphicsCommandPool, nullptr);
		m_GraphicsCommandPool = VK_NULL_HANDLE;	
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

	m_ImageAvailableSemaphores.resize(g_MaxFramesInFlight, VK_NULL_HANDLE);
	m_RenderFinishedSemaphores.resize(g_MaxFramesInFlight, VK_NULL_HANDLE);
	m_Fences.resize(g_MaxFramesInFlight, VK_NULL_HANDLE);

	for (std::uint32_t Iterator = 0u; Iterator < g_MaxFramesInFlight; ++Iterator)
	{
		RENDERCORE_CHECK_VULKAN_RESULT(vkCreateSemaphore(m_Device, &SemaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphores[Iterator]));
		RENDERCORE_CHECK_VULKAN_RESULT(vkCreateSemaphore(m_Device, &SemaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphores[Iterator]));
		RENDERCORE_CHECK_VULKAN_RESULT(vkCreateFence(m_Device, &FenceCreateInfo, nullptr, &m_Fences[Iterator]));
	}

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

	for (const VkSemaphore &SemaphoreIter : m_ImageAvailableSemaphores)
	{
		if (SemaphoreIter != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(m_Device, SemaphoreIter, nullptr);
		}
	}
	m_ImageAvailableSemaphores.clear();

	for (const VkSemaphore &SemaphoreIter : m_RenderFinishedSemaphores)
	{
		if (SemaphoreIter != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(m_Device, SemaphoreIter, nullptr);
		}
	}
	m_RenderFinishedSemaphores.clear();

	for (const VkFence &FenceITer : m_Fences)
	{
		if (FenceITer != VK_NULL_HANDLE)
		{
			vkDestroyFence(m_Device, FenceITer, nullptr);
		}
	}
	m_Fences.clear();

	m_SynchronizationObjectsCreated = false;
}

std::unordered_map<VkSwapchainKHR, std::uint32_t> VulkanCommandsManager::DrawFrame(const std::vector<VkSwapchainKHR> &SwapChains)
{
	if (m_Device == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan logical device is invalid.");
	}

	if (!m_SynchronizationObjectsCreated)
	{
		return std::unordered_map<VkSwapchainKHR, std::uint32_t>();
	}

	std::unordered_map<VkSwapchainKHR, std::uint32_t> ImageIndices;
	for (const VkSwapchainKHR& SwapChainIter : SwapChains)
	{
		if (SwapChainIter == VK_NULL_HANDLE)
		{
			throw std::runtime_error("Vulkan swap chain is invalid.");
		}
	
		WaitAndResetFences(true);

		if (m_ImageAvailableSemaphores[m_CurrentFrameIndex] == VK_NULL_HANDLE)
		{
			throw std::runtime_error("Vulkan semaphore: Image Available is invalid.");
		}

		if (m_Fences[m_CurrentFrameIndex] == VK_NULL_HANDLE)
		{
			throw std::runtime_error("Vulkan fence is invalid.");
		}

		ImageIndices.emplace(SwapChainIter, 0u);
		if (const VkResult OperationResult = vkAcquireNextImageKHR(m_Device, SwapChainIter, Timeout, m_ImageAvailableSemaphores[m_CurrentFrameIndex], m_Fences[m_CurrentFrameIndex], &ImageIndices[SwapChainIter]); OperationResult != VK_SUCCESS)
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
					WaitAndResetFences(true);
				}

				return std::unordered_map<VkSwapchainKHR, std::uint32_t>();
			}
			else if (OperationResult != VK_SUBOPTIMAL_KHR)
			{
				throw std::runtime_error("Failed to acquire Vulkan swap chain image.");
			}
		}
	}

	return ImageIndices;
}

void VulkanCommandsManager::RecordCommandBuffers(const BufferRecordParameters &Parameters)
{
	if (Parameters.Pipeline == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan graphics pipeline is invalid");
	}
	
	if (m_CommandBuffers.empty())
	{
		AllocateCommandBuffers();
	}

	const VkCommandBufferBeginInfo CommandBufferBeginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

	RENDERCORE_CHECK_VULKAN_RESULT(vkBeginCommandBuffer(m_CommandBuffers.back(), &CommandBufferBeginInfo));

	const VkImageMemoryBarrier Barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = Parameters.Image,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0u,
			.levelCount = 1u,
			.baseArrayLayer = 0u,
			.layerCount = 1u}};

	vkCmdPipelineBarrier(
		m_CommandBuffers.back(),
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0u, 0u, nullptr, 0u, nullptr, 1u, &Barrier);

	const VkClearValue ClearValue{
		.color = {0.f, 0.f, 0.f, 1.f}};

	const VkRenderPassBeginInfo RenderPassBeginInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = Parameters.RenderPass,
		.framebuffer = Parameters.FrameBuffers[Parameters.ImageIndex],
		.renderArea = {
			.offset = {0, 0},
			.extent = Parameters.Extent},
		.clearValueCount = 1u,
		.pClearValues = &ClearValue};

	vkCmdBeginRenderPass(m_CommandBuffers.back(), &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(m_CommandBuffers.back(), VK_PIPELINE_BIND_POINT_GRAPHICS, Parameters.Pipeline);

	vkCmdBindVertexBuffers(m_CommandBuffers.back(), 0u, 1u, Parameters.VertexBuffers.data(), Parameters.Offsets.data());

	for (const VkBuffer &IndexBufferIter : Parameters.IndexBuffers)
	{
		vkCmdBindIndexBuffer(m_CommandBuffers.back(), IndexBufferIter, 0u, VK_INDEX_TYPE_UINT16);
	}

	const VkViewport Viewport{
		.x = 0.f,
		.y = 0.f,
		.width = static_cast<float>(Parameters.Extent.width),
		.height = static_cast<float>(Parameters.Extent.height),
		.minDepth = 0.f,
		.maxDepth = 1.f
	};

	vkCmdSetViewport(m_CommandBuffers.back(), 0u, 1u, &Viewport);

	const VkRect2D Scissor{
		.offset = {0, 0},
		.extent = Parameters.Extent};

	vkCmdSetScissor(m_CommandBuffers.back(), 0u, 1u, &Scissor);

	vkCmdBindDescriptorSets(m_CommandBuffers.back(), VK_PIPELINE_BIND_POINT_GRAPHICS, Parameters.PipelineLayout, 0u, 1u, &Parameters.DescriptorSets[m_CurrentFrameIndex], 0u, nullptr);

	vkCmdDrawIndexed(m_CommandBuffers.back(), Parameters.IndexCount, 1u, 0u, 0u, 0u);
	vkCmdEndRenderPass(m_CommandBuffers.back());

	RENDERCORE_CHECK_VULKAN_RESULT(vkEndCommandBuffer(m_CommandBuffers.back()));
}

void VulkanCommandsManager::SubmitCommandBuffers(const VkQueue &GraphicsQueue)
{
	if (GraphicsQueue == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan graphics queue is invalid.");
	}

	WaitAndResetFences(true);

	constexpr VkPipelineStageFlags WaitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

	const VkSubmitInfo SubmitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1u,
		.pWaitSemaphores = &m_ImageAvailableSemaphores[m_CurrentFrameIndex],
		.pWaitDstStageMask = WaitStages,
		.commandBufferCount = 1u,
		.pCommandBuffers = &m_CommandBuffers.back(),
		.signalSemaphoreCount = 1u,
		.pSignalSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrameIndex]};
	
	RENDERCORE_CHECK_VULKAN_RESULT(vkQueueSubmit(GraphicsQueue, 1u, &SubmitInfo, m_Fences[m_CurrentFrameIndex]));
    RENDERCORE_CHECK_VULKAN_RESULT(vkQueueWaitIdle(GraphicsQueue));

	vkFreeCommandBuffers(m_Device, m_GraphicsCommandPool, 1u, &m_CommandBuffers.back());
	m_CommandBuffers.pop_back();
}

void VulkanCommandsManager::PresentFrame(const VkQueue &PresentQueue, const std::unordered_map<VkSwapchainKHR, std::uint32_t> &ImageIndicesData)
{
	if (PresentQueue == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan present queue is invalid.");
	}

	std::vector<VkSwapchainKHR> SwapChains;
	std::vector<std::uint32_t> ImageIndices;
	for (const auto &ImageIndicesIter : ImageIndicesData)
	{
		SwapChains.push_back(ImageIndicesIter.first);
		ImageIndices.push_back(ImageIndicesIter.second);
	}

	const VkPresentInfoKHR PresentInfo{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1u,
		.pWaitSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrameIndex],
		.swapchainCount = static_cast<std::uint32_t>(SwapChains.size()),
		.pSwapchains = SwapChains.data(),
		.pImageIndices = ImageIndices.data(),
		.pResults = nullptr};

	if (const VkResult OperationResult = vkQueuePresentKHR(PresentQueue, &PresentInfo); OperationResult != VK_SUCCESS)
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
	m_GraphicsCommandPool = CreateCommandPool(m_GraphicsProcessingFamilyQueueIndex.value());
}

void VulkanCommandsManager::AllocateCommandBuffers()
{
	if (m_GraphicsCommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(m_Device, m_GraphicsCommandPool, nullptr);
		m_GraphicsCommandPool = VK_NULL_HANDLE;
	}

	if (!m_CommandBuffers.empty())
	{
		vkFreeCommandBuffers(m_Device, m_GraphicsCommandPool, static_cast<std::uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());
		m_CommandBuffers.clear();
	}

	CreateGraphicsCommandPool();
	m_CommandBuffers.resize(g_MaxFramesInFlight, VK_NULL_HANDLE);

	const VkCommandBufferAllocateInfo CommandBufferAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = m_GraphicsCommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = static_cast<std::uint32_t>(m_CommandBuffers.size())};

	RENDERCORE_CHECK_VULKAN_RESULT(vkAllocateCommandBuffers(m_Device, &CommandBufferAllocateInfo, m_CommandBuffers.data()));
}

void VulkanCommandsManager::WaitAndResetFences(const bool bCurrentFrame)
{
	if (m_Fences.empty())
	{
		return;
	}

	if (bCurrentFrame && m_Fences.size() > m_CurrentFrameIndex && m_Fences[m_CurrentFrameIndex] != VK_NULL_HANDLE)
	{
		RENDERCORE_CHECK_VULKAN_RESULT(vkWaitForFences(m_Device, 1u, &m_Fences[m_CurrentFrameIndex], VK_TRUE, Timeout));
		RENDERCORE_CHECK_VULKAN_RESULT(vkResetFences(m_Device, 1u, &m_Fences[m_CurrentFrameIndex]));
	}
	else if (!bCurrentFrame)
	{
		RENDERCORE_CHECK_VULKAN_RESULT(vkWaitForFences(m_Device, static_cast<std::uint32_t>(m_Fences.size()), m_Fences.data(), VK_TRUE, Timeout));
		RENDERCORE_CHECK_VULKAN_RESULT(vkResetFences(m_Device, static_cast<std::uint32_t>(m_Fences.size()), m_Fences.data()));
	}
}
