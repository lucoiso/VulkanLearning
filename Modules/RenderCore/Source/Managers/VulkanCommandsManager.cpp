// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "Managers/VulkanCommandsManager.h"
#include "Managers/VulkanDeviceManager.h"
#include "Managers/VulkanPipelineManager.h"
#include "Managers/VulkanBufferManager.h"
#include "Utils/RenderCoreHelpers.h"
#include "Utils/VulkanConstants.h"
#include <boost/log/trivial.hpp>
#include "VulkanCommandsManager.h"

using namespace RenderCore;

VulkanCommandsManager VulkanCommandsManager::g_Instance{};

VulkanCommandsManager::VulkanCommandsManager()
	: m_CommandPool(VK_NULL_HANDLE)
	, m_CommandBuffer(VK_NULL_HANDLE)
	, m_ImageAvailableSemaphore(VK_NULL_HANDLE)
	, m_RenderFinishedSemaphore(VK_NULL_HANDLE)
	, m_Fence(VK_NULL_HANDLE)
	, m_SynchronizationObjectsCreated(false)
	, m_FrameIndex(0u)
{
}

VulkanCommandsManager::~VulkanCommandsManager()
{
	Shutdown();
}

VulkanCommandsManager &VulkanCommandsManager::Get()
{
	return g_Instance;
}

void VulkanCommandsManager::Shutdown()
{
	BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down Vulkan commands manager";

	WaitAndResetFences();

	const VkDevice &VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

	if (m_CommandBuffer != VK_NULL_HANDLE)
	{
		vkFreeCommandBuffers(VulkanLogicalDevice, m_CommandPool, 1u, &m_CommandBuffer);
		m_CommandBuffer = VK_NULL_HANDLE;
	}

	if (m_CommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(VulkanLogicalDevice, m_CommandPool, nullptr);
		m_CommandPool = VK_NULL_HANDLE;	
	}

	DestroySynchronizationObjects();
}

VkCommandPool VulkanCommandsManager::CreateCommandPool(const std::uint8_t FamilyQueueIndex)
{
	const VkCommandPoolCreateInfo CommandPoolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
		.queueFamilyIndex = static_cast<std::uint32_t>(FamilyQueueIndex)};

	VkCommandPool Output = VK_NULL_HANDLE;
	RENDERCORE_CHECK_VULKAN_RESULT(vkCreateCommandPool(VulkanDeviceManager::Get().GetLogicalDevice(), &CommandPoolCreateInfo, nullptr, &Output));

	return Output;
}

void VulkanCommandsManager::CreateSynchronizationObjects()
{
	if (m_SynchronizationObjectsCreated)
	{
		return;
	}

	BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating Vulkan synchronization objects";

	const VkSemaphoreCreateInfo SemaphoreCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

	const VkFenceCreateInfo FenceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT};

	const VkDevice &VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

	RENDERCORE_CHECK_VULKAN_RESULT(vkCreateSemaphore(VulkanLogicalDevice, &SemaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphore));
	RENDERCORE_CHECK_VULKAN_RESULT(vkCreateSemaphore(VulkanLogicalDevice, &SemaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphore));
	RENDERCORE_CHECK_VULKAN_RESULT(vkCreateFence(VulkanLogicalDevice, &FenceCreateInfo, nullptr, &m_Fence));

	m_SynchronizationObjectsCreated = true;
}

void VulkanCommandsManager::DestroySynchronizationObjects()
{
	if (!m_SynchronizationObjectsCreated)
	{
		return;
	}

	BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying Vulkan synchronization objects";

	const VkDevice &VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

	vkDeviceWaitIdle(VulkanLogicalDevice);

	if (m_CommandBuffer != VK_NULL_HANDLE)
	{
		vkFreeCommandBuffers(VulkanLogicalDevice, m_CommandPool, 1u, &m_CommandBuffer);
		m_CommandBuffer = VK_NULL_HANDLE;
	}

	if (m_CommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(VulkanLogicalDevice, m_CommandPool, nullptr);
		m_CommandPool = VK_NULL_HANDLE;
	}

	if (m_ImageAvailableSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(VulkanLogicalDevice, m_ImageAvailableSemaphore, nullptr);
		m_ImageAvailableSemaphore = VK_NULL_HANDLE;
	}

	if (m_RenderFinishedSemaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(VulkanLogicalDevice, m_RenderFinishedSemaphore, nullptr);
		m_RenderFinishedSemaphore = VK_NULL_HANDLE;
	}

	if (m_Fence != VK_NULL_HANDLE)
	{
		vkDestroyFence(VulkanLogicalDevice, m_Fence, nullptr);
		m_Fence = VK_NULL_HANDLE;
	}

	m_SynchronizationObjectsCreated = false;
}

std::optional<std::int32_t> VulkanCommandsManager::DrawFrame()
{
	if (!m_SynchronizationObjectsCreated)
	{
		return -1;
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
	const VkResult OperationResult = vkAcquireNextImageKHR(VulkanDeviceManager::Get().GetLogicalDevice(), VulkanBufferManager::Get().GetSwapChain(), Timeout, m_ImageAvailableSemaphore, m_Fence, &Output);
	if (OperationResult != VK_SUCCESS)
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

			return std::optional<std::int32_t>();
		}
		else if (OperationResult != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to acquire Vulkan swap chain image.");
		}
	}

	return static_cast<std::int32_t>(Output);
}

void VulkanCommandsManager::RecordCommandBuffers(const std::uint32_t ImageIndex)
{	
	AllocateCommandBuffer();

	const VkCommandBufferBeginInfo CommandBufferBeginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

	RENDERCORE_CHECK_VULKAN_RESULT(vkBeginCommandBuffer(m_CommandBuffer, &CommandBufferBeginInfo));

	const VkRenderPass &RenderPass = VulkanPipelineManager::Get().GetRenderPass();
	const VkPipeline &Pipeline = VulkanPipelineManager::Get().GetPipeline();
	const VkPipelineLayout &PipelineLayout = VulkanPipelineManager::Get().GetPipelineLayout();
	const std::vector<VkDescriptorSet> &DescriptorSets = VulkanPipelineManager::Get().GetDescriptorSets();

	const std::vector<VkFramebuffer> &FrameBuffers = VulkanBufferManager::Get().GetFrameBuffers();
	const VkBuffer &VertexBuffer = VulkanBufferManager::Get().GetVertexBuffer();
	const VkBuffer &IndexBuffer = VulkanBufferManager::Get().GetIndexBuffer();
	const std::uint32_t IndexCount = VulkanBufferManager::Get().GetIndicesCount();
	const UniformBufferObject UniformBufferObj = RenderCoreHelpers::GetUniformBufferObject();

	const std::vector<VkDeviceSize> Offsets = { 0u };
	const VkExtent2D Extent = VulkanBufferManager::Get().GetSwapChainExtent();	

	const VkViewport Viewport{
		.x = 0.f,
		.y = 0.f,
		.width = static_cast<float>(Extent.width),
		.height = static_cast<float>(Extent.height),
		.minDepth = 0.f,
		.maxDepth = 1.f};

	const VkRect2D Scissor{
		.offset = {0, 0},
		.extent = Extent};

	bool bActiveRenderPass = false;
	if (RenderPass != VK_NULL_HANDLE && !FrameBuffers.empty())
	{
		const VkRenderPassBeginInfo RenderPassBeginInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = RenderPass,
			.framebuffer = FrameBuffers[ImageIndex],
			.renderArea = {
				.offset = {0, 0},
				.extent = Extent},
			.clearValueCount = static_cast<std::uint32_t>(g_ClearValues.size()),
			.pClearValues = g_ClearValues.data()};

		vkCmdBeginRenderPass(m_CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		bActiveRenderPass = true;
	}

	if (Pipeline != VK_NULL_HANDLE)
	{
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline);
	}

	if (!DescriptorSets.empty())
	{
		const VkDescriptorSet &DescriptorSet = DescriptorSets[m_FrameIndex];
		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout, 0u, 1u, &DescriptorSet, 0u, nullptr);
	}
	
	vkCmdPushConstants(m_CommandBuffer, PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0u, sizeof(UniformBufferObject), &UniformBufferObj);

	bool bActiveVertexBinding = false;
	if (VertexBuffer != VK_NULL_HANDLE)
	{
		vkCmdBindVertexBuffers(m_CommandBuffer, 0u, 1u, &VertexBuffer, Offsets.data());
		bActiveVertexBinding = true;
	}

	bool bActiveIndexBinding = false;
	if (IndexBuffer != VK_NULL_HANDLE)
	{
		vkCmdBindIndexBuffer(m_CommandBuffer, IndexBuffer, 0u, VK_INDEX_TYPE_UINT32);
		bActiveIndexBinding = true;	
	}

	vkCmdSetViewport(m_CommandBuffer, 0u, 1u, &Viewport);
	vkCmdSetScissor(m_CommandBuffer, 0u, 1u, &Scissor);

	if (bActiveRenderPass && bActiveVertexBinding && bActiveIndexBinding)
	{
		vkCmdDrawIndexed(m_CommandBuffer, IndexCount, 1u, 0u, 0u, 0u);
	}

	if (bActiveRenderPass)
	{		
		vkCmdEndRenderPass(m_CommandBuffer);
	}

	RENDERCORE_CHECK_VULKAN_RESULT(vkEndCommandBuffer(m_CommandBuffer));
}

void VulkanCommandsManager::SubmitCommandBuffers()
{
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

	const VkQueue &GraphicsQueue = VulkanDeviceManager::Get().GetGraphicsQueue().second;
	
	RENDERCORE_CHECK_VULKAN_RESULT(vkQueueSubmit(GraphicsQueue, 1u, &SubmitInfo, m_Fence));
    RENDERCORE_CHECK_VULKAN_RESULT(vkQueueWaitIdle(GraphicsQueue));

	vkFreeCommandBuffers(VulkanDeviceManager::Get().GetLogicalDevice(), m_CommandPool, 1u, &m_CommandBuffer);
	m_CommandBuffer = VK_NULL_HANDLE;
}

void VulkanCommandsManager::PresentFrame(const std::uint32_t ImageIndice)
{
	const VkPresentInfoKHR PresentInfo{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1u,
		.pWaitSemaphores = &m_RenderFinishedSemaphore,
		.swapchainCount = 1u,
		.pSwapchains = &VulkanBufferManager::Get().GetSwapChain(),
		.pImageIndices = &ImageIndice,
		.pResults = nullptr};

	if (const VkResult OperationResult = vkQueuePresentKHR(VulkanDeviceManager::Get().GetGraphicsQueue().second, &PresentInfo); OperationResult != VK_SUCCESS)
	{
		if (OperationResult != VK_ERROR_OUT_OF_DATE_KHR && OperationResult != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Vulkan operation failed with result: " + std::string(ResultToString(OperationResult)));
		}
	}
	
    m_FrameIndex = (m_FrameIndex + 1u) % g_MaxFramesInFlight;
}

void VulkanCommandsManager::CreateGraphicsCommandPool()
{
	m_CommandPool = CreateCommandPool(VulkanDeviceManager::Get().GetGraphicsQueue().first);
}

void VulkanCommandsManager::AllocateCommandBuffer()
{
	const VkDevice &VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

	if (m_CommandBuffer != VK_NULL_HANDLE)
	{
		vkFreeCommandBuffers(VulkanLogicalDevice, m_CommandPool, 1u, &m_CommandBuffer);
		m_CommandBuffer = VK_NULL_HANDLE;
	}

	if (m_CommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(VulkanLogicalDevice, m_CommandPool, nullptr);
		m_CommandPool = VK_NULL_HANDLE;
	}

	CreateGraphicsCommandPool();

	const VkCommandBufferAllocateInfo CommandBufferAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = m_CommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1u};

	RENDERCORE_CHECK_VULKAN_RESULT(vkAllocateCommandBuffers(VulkanLogicalDevice, &CommandBufferAllocateInfo, &m_CommandBuffer));
}

void VulkanCommandsManager::WaitAndResetFences()
{
	if (m_Fence == VK_NULL_HANDLE)
	{
		return;
	}

	const VkDevice &VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

	RENDERCORE_CHECK_VULKAN_RESULT(vkWaitForFences(VulkanLogicalDevice, 1u, &m_Fence, VK_TRUE, Timeout));
	RENDERCORE_CHECK_VULKAN_RESULT(vkResetFences(VulkanLogicalDevice, 1u, &m_Fence));
}
