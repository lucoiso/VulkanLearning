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
	, m_CommandPools({})
	, m_CommandBuffers({})
	, m_ImageAvailableSemaphores({})
	, m_RenderFinishedSemaphores({})
	, m_Fences({})
	, m_CurrentFrameIndex(0u)
	, m_SynchronizationObjectsCreated(false)
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

	for (const auto& CommandPoolIter : m_CommandPools)
	{
		if (CommandPoolIter.first == m_GraphicsProcessingFamilyQueueIndex)
		{
			vkFreeCommandBuffers(m_Device, CommandPoolIter.second, static_cast<std::uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());
			m_CommandBuffers.clear();
		}

		vkDestroyCommandPool(m_Device, CommandPoolIter.second, nullptr);
	}
	m_CommandPools.clear();

	DestroySynchronizationObjects();
}

void VulkanCommandsManager::CreateCommandPool(const std::uint32_t FamilyQueueIndex, const bool bIsGraphicsIndex)
{
	BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan command pool";

	if (m_Device == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan logical device is invalid.");
	}

	const VkCommandPoolCreateInfo CommandPoolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = FamilyQueueIndex};

	VkCommandPool CommandPool = VK_NULL_HANDLE;
	RENDERCORE_CHECK_VULKAN_RESULT(vkCreateCommandPool(m_Device, &CommandPoolCreateInfo, nullptr, &CommandPool));

	m_CommandPools.emplace(FamilyQueueIndex, CommandPool);

	if (bIsGraphicsIndex)
	{
		m_GraphicsProcessingFamilyQueueIndex = FamilyQueueIndex;
	}
}

void VulkanCommandsManager::CreateCommandBuffers()
{
	BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan command buffers";

	m_CommandBuffers.resize(g_MaxFramesInFlight, VK_NULL_HANDLE);

	const VkCommandBufferAllocateInfo CommandBufferAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = m_CommandPools.at(m_GraphicsProcessingFamilyQueueIndex),
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = static_cast<std::uint32_t>(m_CommandBuffers.size())};

	RENDERCORE_CHECK_VULKAN_RESULT(vkAllocateCommandBuffers(m_Device, &CommandBufferAllocateInfo, m_CommandBuffers.data()));
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
			
	RENDERCORE_CHECK_VULKAN_RESULT(vkResetCommandBuffer(m_CommandBuffers[m_CurrentFrameIndex], 0u));

	return ImageIndices;
}

void VulkanCommandsManager::RecordCommandBuffers(const BufferRecordParameters &Parameters)
{
	if (Parameters.Pipeline == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan graphics pipeline is invalid");
	}

	const VkCommandBufferBeginInfo CommandBufferBeginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

	const std::vector<VkClearValue> ClearValues{
		VkClearValue{
			.color = {0.f, 0.f, 0.f, 1.f}}};

	RENDERCORE_CHECK_VULKAN_RESULT(vkBeginCommandBuffer(m_CommandBuffers[m_CurrentFrameIndex], &CommandBufferBeginInfo));

	const VkRenderPassBeginInfo RenderPassBeginInfo{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = Parameters.RenderPass,
		.framebuffer = Parameters.FrameBuffers[Parameters.ImageIndex],
		.renderArea = {
			.offset = {0, 0},
			.extent = Parameters.Extent},
		.clearValueCount = static_cast<std::uint32_t>(ClearValues.size()),
		.pClearValues = ClearValues.data()};

	vkCmdBeginRenderPass(m_CommandBuffers[m_CurrentFrameIndex], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(m_CommandBuffers[m_CurrentFrameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, Parameters.Pipeline);

	vkCmdBindVertexBuffers(m_CommandBuffers[m_CurrentFrameIndex], 0u, 1u, Parameters.VertexBuffers.data(), Parameters.Offsets.data());

	for (const VkBuffer &IndexBufferIter : Parameters.IndexBuffers)
	{
		vkCmdBindIndexBuffer(m_CommandBuffers[m_CurrentFrameIndex], IndexBufferIter, 0u, VK_INDEX_TYPE_UINT16);
	}

	const VkViewport Viewport{
		.x = 0.f,
		.y = 0.f,
		.width = static_cast<float>(Parameters.Extent.width),
		.height = static_cast<float>(Parameters.Extent.height),
		.minDepth = 0.f,
		.maxDepth = 1.f
	};

	vkCmdSetViewport(m_CommandBuffers[m_CurrentFrameIndex], 0u, 1u, &Viewport);

	const VkRect2D Scissor{
		.offset = {0, 0},
		.extent = Parameters.Extent};

	vkCmdSetScissor(m_CommandBuffers[m_CurrentFrameIndex], 0u, 1u, &Scissor);

	vkCmdBindDescriptorSets(m_CommandBuffers[m_CurrentFrameIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, Parameters.PipelineLayout, 0u, 1u, &Parameters.DescriptorSets[m_CurrentFrameIndex], 0u, nullptr);

	vkCmdDrawIndexed(m_CommandBuffers[m_CurrentFrameIndex], Parameters.IndexCount, 1u, 0u, 0u, 0u);
	vkCmdEndRenderPass(m_CommandBuffers[m_CurrentFrameIndex]);

	RENDERCORE_CHECK_VULKAN_RESULT(vkEndCommandBuffer(m_CommandBuffers[m_CurrentFrameIndex]));
}

void VulkanCommandsManager::SubmitCommandBuffers(const VkQueue &GraphicsQueue)
{
	if (GraphicsQueue == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan graphics queue is invalid.");
	}

	WaitAndResetFences(true);

	constexpr VkPipelineStageFlags WaitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

	const std::vector<VkSubmitInfo> SubmitInfo{
		VkSubmitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1u,
			.pWaitSemaphores = &m_ImageAvailableSemaphores[m_CurrentFrameIndex],
			.pWaitDstStageMask = WaitStages,
			.commandBufferCount = 1u,
			.pCommandBuffers = &m_CommandBuffers[m_CurrentFrameIndex],
			.signalSemaphoreCount = 1u,
			.pSignalSemaphores = &m_RenderFinishedSemaphores[m_CurrentFrameIndex]}};
	
	RENDERCORE_CHECK_VULKAN_RESULT(vkQueueSubmit(GraphicsQueue, static_cast<std::uint32_t>(SubmitInfo.size()), SubmitInfo.data(), m_Fences[m_CurrentFrameIndex]));
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
		SwapChains.emplace_back(ImageIndicesIter.first);
		ImageIndices.emplace_back(ImageIndicesIter.second);
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

std::uint32_t RenderCore::VulkanCommandsManager::GetCurrentFrameIndex() const
{
    return m_CurrentFrameIndex;
}

bool VulkanCommandsManager::IsInitialized() const
{
	return !m_CommandPools.empty();
}

const VkCommandPool &VulkanCommandsManager::GetCommandPool(const std::uint32_t FamilyIndex) const
{
	return m_CommandPools.at(FamilyIndex);
}

const std::vector<VkCommandBuffer> &VulkanCommandsManager::GetCommandBuffers() const
{
	return m_CommandBuffers;
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
