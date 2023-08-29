// Copyright Notice: [...]

#include "VulkanPipelineManager.h"
#include "VulkanShaderCompiler.h"
#include <boost/log/trivial.hpp>

using namespace RenderCore;

VulkanPipelineManager::VulkanPipelineManager(const VkInstance& Instance, const VkDevice& Device)
    : m_Instance(Instance)
    , m_Device(Device)
    , m_Pipelines({})
    , m_ShaderCompiler(std::make_unique<VulkanShaderCompiler>())
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan pipelines manager";
}

VulkanPipelineManager::~VulkanPipelineManager()
{
    if (!IsInitialized())
    {
       return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing vulkan pipelines manager";
    Shutdown();
}

void VulkanPipelineManager::Initialize(const VkRenderPass& RenderPass, const VkExtent2D& Extent)
{
    if (IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(warning) << "[" << __func__ << "]: Commented render pass verification for testing. Location " << __FILE__ << ":" << __LINE__;
    // if (RenderPass == VK_NULL_HANDLE)
    // {
    //     throw std::runtime_error("Invalid vulkan render pass");
    // }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan pipelines";
}

void VulkanPipelineManager::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan pipelines";

    for (const VkPipeline& PipelineIter : m_Pipelines)
    {
        if (PipelineIter != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(m_Device, PipelineIter, nullptr);
        }
    }
    m_Pipelines.clear();
}

bool VulkanPipelineManager::IsInitialized() const
{
    return m_Instance   != VK_NULL_HANDLE
        && m_Device     != VK_NULL_HANDLE;
}

const std::vector<VkPipeline>& RenderCore::VulkanPipelineManager::GetPipelines() const
{
    return m_Pipelines;
}
