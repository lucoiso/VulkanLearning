// Copyright Notice: [...]

#include "VulkanPipelineManager.h"
#include "VulkanShaderCompiler.h"
#include <boost/log/trivial.hpp>

using namespace RenderCore;

VulkanPipelineManager::VulkanPipelineManager()
    : m_Pipelines()
    , m_Device(VK_NULL_HANDLE)
    , m_ShaderCompiler(std::make_unique<VulkanShaderCompiler>())
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan pipeline manager";
}

VulkanPipelineManager::~VulkanPipelineManager()
{
    if (IsInitialized())
    {
        BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destructing vulkan pipeline manager";
        Shutdown();
    }
}

void VulkanPipelineManager::Initialize(const VkDevice& Device, const VkRenderPass& RenderPass, const VkExtent2D& Extent)
{
    if (IsInitialized())
    {
        return;
    }

    if (Device == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Invalid vulkan logical device");
    }

    // if (RenderPass == VK_NULL_HANDLE)
    // {
    //     throw std::runtime_error("Invalid vulkan render pass");
    // }

    #ifdef DEBUG_SHADER_FRAG
    CompileShader(DEBUG_SHADER_FRAG);
    #endif

    #ifdef DEBUG_SHADER_VERT
    CompileShader(DEBUG_SHADER_VERT);
    #endif

    m_Device = Device;
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan pipelines";
}

void VulkanPipelineManager::Shutdown()
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

void VulkanPipelineManager::CompileShader(const char* ShaderSource)
{
    if (m_ShaderCompiler == nullptr)
    {
        throw std::runtime_error("Invalid shader compiler");
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Running test function to compile shader. Location: " << __FILE__ << ":" << __LINE__;

    std::vector<uint32_t> SPIRVCode;
    if (!m_ShaderCompiler->Compile(ShaderSource, SPIRVCode))
    {
        const std::string ErrMessage = "Failed to compile shader: " + std::string(ShaderSource);
        throw std::runtime_error(ErrMessage);
    }
}

bool VulkanPipelineManager::IsInitialized() const
{
    return !m_Pipelines.empty() && m_Device != VK_NULL_HANDLE;
}