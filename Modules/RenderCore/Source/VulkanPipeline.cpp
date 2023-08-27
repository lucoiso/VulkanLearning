// Copyright Notice: [...]

#include "VulkanPipeline.h"
#include "VulkanShaderCompiler.h"
#include <boost/log/trivial.hpp>

using namespace RenderCore;

VulkanPipeline::VulkanPipeline()
    : m_Pipelines()
    , m_Device(VK_NULL_HANDLE)
    , m_ShaderCompiler(std::make_unique<VulkanShaderCompiler>())
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan pipeline manager";
}

VulkanPipeline::~VulkanPipeline()
{
    if (IsInitialized())
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing vulkan pipeline manager";
        Shutdown();
    }
}

void VulkanPipeline::Initialize(const VkDevice& Device, const VkRenderPass& RenderPass, const VkExtent2D& Extent)
{
    if (IsInitialized())
    {
        return;
    }

    if (Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Invalid vulkan logical device");
    }

    if (RenderPass == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Invalid vulkan render pass");
    }

    m_Device = Device;
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan pipelines";
}

void VulkanPipeline::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying vulkan pipelines";
    for (const VkPipeline& PipelineIter : m_Pipelines)
    {
        if (PipelineIter != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(m_Device, PipelineIter, nullptr);
        }
    }
    m_Pipelines.clear();
}

bool VulkanPipeline::IsInitialized() const
{
    return !m_Pipelines.empty() && m_Device != VK_NULL_HANDLE;
}