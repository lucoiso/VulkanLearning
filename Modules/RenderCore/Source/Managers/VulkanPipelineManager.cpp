// Copyright Notice: [...]

#include "Managers/VulkanPipelineManager.h"
#include "Utils/VulkanConstants.h"
#include <boost/log/trivial.hpp>

using namespace RenderCore;

VulkanPipelineManager::VulkanPipelineManager(const VkInstance& Instance, const VkDevice& Device)
    : m_Instance(Instance)
    , m_Device(Device)
    , m_RenderPass(VK_NULL_HANDLE)
    , m_Pipeline(VK_NULL_HANDLE)
    , m_PipelineLayout(VK_NULL_HANDLE)
    // , m_PipelineCache(VK_NULL_HANDLE)
    // , m_DescriptorSetLayout(VK_NULL_HANDLE)
    , m_Viewport {
            .x = 0.f,
            .y = 0.f,
            .width = 0.f,
            .height = 0.f,
            .minDepth = 0.f,
            .maxDepth = 1.f
        }
    , m_Scissor{
            .offset = { 0, 0 },
            .extent = { 0, 0 }
        }
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

void VulkanPipelineManager::CreateRenderPass(const VkFormat& Format)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan render pass";

    const VkAttachmentDescription ColorAttachment{
        .format = Format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    const VkAttachmentReference ColorAttachmentRef {
        .attachment = 0u,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    const VkSubpassDescription Subpass{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1u,
        .pColorAttachments = &ColorAttachmentRef
    };

    const VkSubpassDependency Dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0u,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0u,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };

    const VkRenderPassCreateInfo RenderPassCreateInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1u,
        .pAttachments = &ColorAttachment,
        .subpassCount = 1u,
        .pSubpasses = &Subpass,
        .dependencyCount = 1u,
        .pDependencies = &Dependency
    };

    if (vkCreateRenderPass(m_Device, &RenderPassCreateInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create vulkan render pass");
    }
}

void VulkanPipelineManager::CreateGraphicsPipeline(const std::vector<VkPipelineShaderStageCreateInfo>& ShaderStages, const VkExtent2D& Extent)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan graphics pipeline";

    // const VkPipelineDynamicStateCreateInfo DynamicState{
    //     .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    //     .dynamicStateCount = static_cast<uint32_t>(g_DynamicStates.size()),
    //     .pDynamicStates = g_DynamicStates.data()
    // };

    // const std::vector<VkVertexInputBindingDescription> BindingDescription{
    //     VkVertexInputBindingDescription{
    //         .binding = 0,
    //         .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    //     },
    //     VkVertexInputBindingDescription{
    //         .binding = 1,
    //         .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    //     }
    // };

    // const std::vector<VkVertexInputAttributeDescription> AttributeDescriptions{
    //     VkVertexInputAttributeDescription{
    //         .location = 0,
    //         .binding = 0,
    //         .format = VK_FORMAT_R32G32B32_SFLOAT
    //     },
    //     VkVertexInputAttributeDescription{
    //         .location = 1,
    //         .binding = 1,
    //         .format = VK_FORMAT_R32G32B32_SFLOAT
    //     }
    // };

    const VkPipelineVertexInputStateCreateInfo VertexInputState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0u,
        .vertexAttributeDescriptionCount = 0u
        // .vertexBindingDescriptionCount = static_cast<std::uint32_t>(BindingDescription.size()),
        // .pVertexBindingDescriptions = BindingDescription.data(),
        // .vertexAttributeDescriptionCount = static_cast<std::uint32_t>(AttributeDescriptions.size()),
        // .pVertexAttributeDescriptions = AttributeDescriptions.data()
    };

    const VkPipelineInputAssemblyStateCreateInfo InputAssemblyState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    const VkPipelineRasterizationStateCreateInfo RasterizationState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f
    };

    const VkPipelineColorBlendAttachmentState ColorBlendAttachment{
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo ColorBlendState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1u,
        .pAttachments = &ColorBlendAttachment
    };

    ColorBlendState.blendConstants[0] = 0.0f;
    ColorBlendState.blendConstants[1] = 0.0f;
    ColorBlendState.blendConstants[2] = 0.0f;
    ColorBlendState.blendConstants[3] = 0.0f;

    m_Viewport.width = static_cast<float>(Extent.width);
    m_Viewport.height = static_cast<float>(Extent.height);

    m_Scissor.extent = Extent;

    const VkPipelineViewportStateCreateInfo ViewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1u,
        .pViewports = &m_Viewport,
        .scissorCount = 1u,
        .pScissors = &m_Scissor
    };

    const VkPipelineMultisampleStateCreateInfo MultisampleState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
    };

    const VkPipelineDepthStencilStateCreateInfo DepthStencilState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_FALSE,
        .depthWriteEnable = VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE
    };

    // const VkDescriptorSetLayoutBinding LayoutBinding{
    //     .binding = 0u,
    //     .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    //     //.descriptorCount = 1u,
    //     .descriptorCount = 0u,
    //     .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    //     .pImmutableSamplers = nullptr
    // };
    
    // const VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutInfo{
    //     .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    //     // .bindingCount = 1u,
    //     .bindingCount = 0u,
    //     .pBindings = &LayoutBinding
    // };
    // 
    // if (vkCreateDescriptorSetLayout(m_Device, &DescriptorSetLayoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS)
    // {
    //     throw std::runtime_error("Failed to create descriptor set layout");
    // }

    const VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0u,
        // .setLayoutCount = 1u,
        // .pSetLayouts = &m_DescriptorSetLayout,
        .pushConstantRangeCount = 0u
    };

    if (vkCreatePipelineLayout(m_Device, &PipelineLayoutCreateInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create vulkan pipeline layout");
    }

    // const VkPipelineCacheCreateInfo PipelineCacheCreateInfo{
    //     .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO
    // };
    // 
    // if (vkCreatePipelineCache(m_Device, &PipelineCacheCreateInfo, nullptr, &m_PipelineCache) != VK_SUCCESS)
    // {
    //     throw std::runtime_error("Failed to create vulkan pipeline cache");
    // }

    const std::vector<VkGraphicsPipelineCreateInfo> GraphicsPipelineCreateInfo{
        VkGraphicsPipelineCreateInfo{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = static_cast<std::uint32_t>(ShaderStages.size()),
            .pStages = ShaderStages.data(),
            .pVertexInputState = &VertexInputState,
            .pInputAssemblyState = &InputAssemblyState,
            .pViewportState = &ViewportState,
            .pRasterizationState = &RasterizationState,
            .pMultisampleState = &MultisampleState,
            .pDepthStencilState = &DepthStencilState,
            .pColorBlendState = &ColorBlendState,
            // .pDynamicState = &DynamicState,
            .layout = m_PipelineLayout,
            .renderPass = m_RenderPass,
            .subpass = 0u
        }
    };

    if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE /*m_PipelineCache*/, GraphicsPipelineCreateInfo.size(), GraphicsPipelineCreateInfo.data(), nullptr, &m_Pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create vulkan graphics pipeline");
    }
}

void VulkanPipelineManager::Shutdown()
{
    if (!IsInitialized())
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan pipelines";

    vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
    m_RenderPass = VK_NULL_HANDLE;

    vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
    m_Pipeline = VK_NULL_HANDLE;

    vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
    m_PipelineLayout = VK_NULL_HANDLE;

    // vkDestroyPipelineCache(m_Device, m_PipelineCache, nullptr);
    // m_PipelineCache = VK_NULL_HANDLE;
    
    // vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
    // m_DescriptorSetLayout = VK_NULL_HANDLE;
}

bool VulkanPipelineManager::IsInitialized() const
{
    return m_RenderPass != VK_NULL_HANDLE
        && m_Pipeline   != VK_NULL_HANDLE
        && m_PipelineLayout != VK_NULL_HANDLE;
}

const VkRenderPass& VulkanPipelineManager::GetRenderPass() const
{
    return m_RenderPass;
}

const VkPipeline& VulkanPipelineManager::GetPipeline() const
{
    return m_Pipeline;
}

const VkPipelineLayout& VulkanPipelineManager::GetPipelineLayout() const
{
    return m_PipelineLayout;
}

// const VkPipelineCache& VulkanPipelineManager::GetPipelineCache() const
// {
//     return m_PipelineCache;
// }

// const VkDescriptorSetLayout& VulkanPipelineManager::GetDescriptorSetLayout() const
// {
//     return m_DescriptorSetLayout;
// }