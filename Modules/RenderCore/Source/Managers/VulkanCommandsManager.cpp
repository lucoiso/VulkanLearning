// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "Managers/VulkanCommandsManager.h"
#include "Utils/RenderCoreHelpers.h"
#include <boost/log/trivial.hpp>

using namespace RenderCore;

VulkanCommandsManager::VulkanCommandsManager(const VkDevice& Device)
	: m_Device(Device)
	, m_CommandPool(VK_NULL_HANDLE)
	, m_CommandBuffers({})
	, m_ImageAvailableSemaphores({})
	, m_RenderFinishedSemaphores({})
	, m_Fences({})
	, m_ProcessingUnitsCount(0u)
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

void VulkanCommandsManager::Shutdown(const std::vector<VkQueue>& PendingQueues)
{
	if (!IsInitialized())
	{
		return;
	}

	BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down Vulkan commands manager";

	WaitAndResetFences();

	vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<std::uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());
	m_CommandBuffers.clear();

	vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
	m_CommandPool = VK_NULL_HANDLE;

	DestroySynchronizationObjects();
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

	RENDERCORE_CHECK_VULKAN_RESULT(vkCreateCommandPool(m_Device, &CommandPoolCreateInfo, nullptr, &m_CommandPool));

	const VkCommandBufferAllocateInfo CommandBufferAllocateInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = m_CommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = static_cast<std::uint32_t>(FrameBuffers.size())
	};

	m_CommandBuffers.resize(FrameBuffers.size(), VK_NULL_HANDLE);
	RENDERCORE_CHECK_VULKAN_RESULT(vkAllocateCommandBuffers(m_Device, &CommandBufferAllocateInfo, m_CommandBuffers.data()));
}

void VulkanCommandsManager::CreateSynchronizationObjects()
{
	BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan synchronization objects";

	if (m_Device == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan logical device is invalid.");
	}

	const VkSemaphoreCreateInfo SemaphoreCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	const VkFenceCreateInfo FenceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	m_ProcessingUnitsCount = static_cast<std::uint32_t>(m_CommandBuffers.size());

	m_ImageAvailableSemaphores.resize(m_ProcessingUnitsCount, VK_NULL_HANDLE);
	m_RenderFinishedSemaphores.resize(m_ProcessingUnitsCount, VK_NULL_HANDLE);
	m_Fences.resize(m_ProcessingUnitsCount, VK_NULL_HANDLE);

	for (std::uint32_t Iterator = 0u; Iterator < m_ProcessingUnitsCount; ++Iterator)
	{
		RENDERCORE_CHECK_VULKAN_RESULT(vkCreateSemaphore(m_Device, &SemaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphores[Iterator]));
		RENDERCORE_CHECK_VULKAN_RESULT(vkCreateSemaphore(m_Device, &SemaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphores[Iterator]));
		RENDERCORE_CHECK_VULKAN_RESULT(vkCreateFence(m_Device, &FenceCreateInfo, nullptr, &m_Fences[Iterator]));
	}
}

void VulkanCommandsManager::DestroySynchronizationObjects()
{
	BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying Vulkan synchronization objects";

	vkDeviceWaitIdle(m_Device);

	for (const VkSemaphore& SemaphoreIter : m_ImageAvailableSemaphores)
	{
		if (SemaphoreIter != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(m_Device, SemaphoreIter, nullptr);
		}
	}
	m_ImageAvailableSemaphores.clear();

	for (const VkSemaphore& SemaphoreIter : m_RenderFinishedSemaphores)
	{
		if (SemaphoreIter != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(m_Device, SemaphoreIter, nullptr);
		}
	}
	m_RenderFinishedSemaphores.clear();

	for (const VkFence& FenceITer : m_Fences)
	{
		if (FenceITer != VK_NULL_HANDLE)
		{
			vkDestroyFence(m_Device, FenceITer, nullptr);
		}
	}
	m_Fences.clear();

	m_ProcessingUnitsCount = 0u;
}

std::vector<std::uint32_t> VulkanCommandsManager::DrawFrame(const std::vector<VkSwapchainKHR>& SwapChains)
{
	if (m_Device == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan logical device is invalid.");
	}

	if (m_ProcessingUnitsCount == 0u)
	{
		return std::vector<std::uint32_t>();
	}

	WaitAndResetFences();

	std::vector<std::uint32_t> ImageIndexes;
	for (std::uint32_t Iterator = 0u; Iterator < m_ProcessingUnitsCount; ++Iterator)
	{
		if (m_ImageAvailableSemaphores[Iterator] == VK_NULL_HANDLE)
		{
			throw std::runtime_error("Vulkan semaphore: Image Available is invalid.");
		}

		if (m_Fences[Iterator] == VK_NULL_HANDLE)
		{
			throw std::runtime_error("Vulkan fence is invalid.");
		}

		if (SwapChains[Iterator] == VK_NULL_HANDLE)
		{
			throw std::runtime_error("Vulkan swap chain is invalid.");
		}

		ImageIndexes.emplace_back(0u);
		if (const VkResult OperationResult = vkAcquireNextImageKHR(m_Device, SwapChains[Iterator], Timeout, m_ImageAvailableSemaphores[Iterator], m_Fences[Iterator], &ImageIndexes.back()); OperationResult != VK_SUCCESS)
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
				}

				WaitAndResetFences();
				DestroySynchronizationObjects();

				return std::vector<std::uint32_t>();
			}
			else if (OperationResult != VK_SUBOPTIMAL_KHR)
			{
				throw std::runtime_error("Failed to acquire Vulkan swap chain image.");
			}
		}
	}

	return ImageIndexes;
}

void VulkanCommandsManager::RecordCommandBuffers(const VkRenderPass& RenderPass, const VkPipeline& Pipeline, const std::vector<VkViewport>& Viewports, const std::vector<VkRect2D>& Scissors, const VkExtent2D& Extent, const std::vector<VkFramebuffer>& FrameBuffers, const std::vector<VkBuffer>& VertexBuffers, const std::vector<VkDeviceSize>& Offsets)
{
	if (Pipeline == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan graphics pipeline is invalid");
	}

	const VkCommandBufferBeginInfo CommandBufferBeginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
	};

	const std::vector<VkClearValue> ClearValues{
		VkClearValue{
			.color = { 0.0f, 0.0f, 0.0f, 1.0f }
		}
	};

	auto CommandBufferIter = m_CommandBuffers.begin();
	for (const VkFramebuffer& FrameBufferIter : FrameBuffers)
	{
		if (CommandBufferIter == m_CommandBuffers.end())
		{
			break;
		}

		RENDERCORE_CHECK_VULKAN_RESULT(vkResetCommandBuffer(*CommandBufferIter, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
		RENDERCORE_CHECK_VULKAN_RESULT(vkBeginCommandBuffer(*CommandBufferIter, &CommandBufferBeginInfo));

		const VkRenderPassBeginInfo RenderPassBeginInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = RenderPass,
			.framebuffer = FrameBufferIter,
			.renderArea = {
				.offset = { 0, 0 },
				.extent = Extent
			},
			.clearValueCount = static_cast<std::uint32_t>(ClearValues.size()),
			.pClearValues = ClearValues.data()
		};

		vkCmdBeginRenderPass(*CommandBufferIter, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(*CommandBufferIter, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);

		if (!VertexBuffers.empty())
		{
			vkCmdBindVertexBuffers(*CommandBufferIter, 0, 1, VertexBuffers.data(), Offsets.data());
		}

		vkCmdSetViewport(*CommandBufferIter, 0u, static_cast<std::uint32_t>(Viewports.size()), Viewports.data());
		vkCmdSetScissor(*CommandBufferIter, 0u, static_cast<std::uint32_t>(Scissors.size()), Scissors.data());

		vkCmdDraw(*CommandBufferIter, 3, 1, 0, 0);
		vkCmdEndRenderPass(*CommandBufferIter);

		if (vkEndCommandBuffer(*CommandBufferIter) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to record command buffer");
		}

		++CommandBufferIter;
	}
}

void VulkanCommandsManager::SubmitCommandBuffers(const VkQueue& GraphicsQueue)
{
	if (GraphicsQueue == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan graphics queue is invalid.");
	}

	WaitAndResetFences();

	const std::vector<VkPipelineStageFlags> WaitStages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	const std::vector<VkSubmitInfo> SubmitInfo{
		VkSubmitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = static_cast<std::uint32_t>(m_ImageAvailableSemaphores.size()),
			.pWaitSemaphores = m_ImageAvailableSemaphores.data(),
			.pWaitDstStageMask = WaitStages.data(),
			.commandBufferCount = static_cast<std::uint32_t>(m_CommandBuffers.size()),
			.pCommandBuffers = m_CommandBuffers.data(),
			.signalSemaphoreCount = static_cast<std::uint32_t>(m_RenderFinishedSemaphores.size()),
			.pSignalSemaphores = m_RenderFinishedSemaphores.data()
		}
	};

	for (const VkFence& FenceIter : m_Fences)
	{
		if (FenceIter == VK_NULL_HANDLE)
		{
			throw std::runtime_error("Vulkan fence is invalid.");
		}

		RENDERCORE_CHECK_VULKAN_RESULT(vkQueueSubmit(GraphicsQueue, static_cast<std::uint32_t>(SubmitInfo.size()), SubmitInfo.data(), FenceIter));
	}
}

void VulkanCommandsManager::PresentFrame(const VkQueue& PresentQueue, const std::vector<VkSwapchainKHR>& SwapChains, const std::vector<std::uint32_t>& ImageIndexes)
{
	if (PresentQueue == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Vulkan present queue is invalid.");
	}

	const VkPresentInfoKHR PresentInfo{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = static_cast<std::uint32_t>(m_RenderFinishedSemaphores.size()),
		.pWaitSemaphores = m_RenderFinishedSemaphores.data(),
		.swapchainCount = static_cast<std::uint32_t>(SwapChains.size()),
		.pSwapchains = SwapChains.data(),
		.pImageIndices = ImageIndexes.data(),
		.pResults = nullptr
	};

	RENDERCORE_CHECK_VULKAN_RESULT(vkQueuePresentKHR(PresentQueue, &PresentInfo));
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

void VulkanCommandsManager::WaitAndResetFences()
{
	if (m_Fences.empty())
	{
		return;
	}

	RENDERCORE_CHECK_VULKAN_RESULT(vkWaitForFences(m_Device, static_cast<std::uint32_t>(m_Fences.size()), m_Fences.data(), VK_TRUE, Timeout));
	RENDERCORE_CHECK_VULKAN_RESULT(vkResetFences(m_Device, static_cast<std::uint32_t>(m_Fences.size()), m_Fences.data()));
}
