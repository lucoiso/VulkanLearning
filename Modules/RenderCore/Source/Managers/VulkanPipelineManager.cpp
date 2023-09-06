// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "Managers/VulkanPipelineManager.h"
#include "Types/VulkanVertex.h"
#include "Types/VulkanUniformBufferObject.h"
#include "Utils/RenderCoreHelpers.h"
#include "Utils/VulkanConstants.h"
#include <boost/log/trivial.hpp>
#include "VulkanPipelineManager.h"

using namespace RenderCore;

VulkanPipelineManager::VulkanPipelineManager(const VkInstance &Instance, const VkDevice &Device)
    : m_Instance(Instance)
    , m_Device(Device)
    , m_RenderPass(VK_NULL_HANDLE)
    , m_Pipeline(VK_NULL_HANDLE)
    , m_PipelineLayout(VK_NULL_HANDLE)
    , m_PipelineCache(VK_NULL_HANDLE)
    , m_DescriptorPool(VK_NULL_HANDLE)
    , m_DescriptorSetLayout(VK_NULL_HANDLE)
    , m_DescriptorSets({})
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

void VulkanPipelineManager::CreateRenderPass(const VkFormat &Format)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan render pass";

    const VkAttachmentDescription ColorAttachmentDescription{
        .format = Format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};

    const VkAttachmentReference ColorAttachmentReference{
        .attachment = 0u,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    const VkSubpassDescription SubpassDescription{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1u,
        .pColorAttachments = &ColorAttachmentReference};

    const VkSubpassDependency SubpassDependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0u,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0u,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};

    const VkRenderPassCreateInfo RenderPassCreateInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1u,
        .pAttachments = &ColorAttachmentDescription,
        .subpassCount = 1u,
        .pSubpasses = &SubpassDescription,
        .dependencyCount = 1u,
        .pDependencies = &SubpassDependency};

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateRenderPass(m_Device, &RenderPassCreateInfo, nullptr, &m_RenderPass));
}

void VulkanPipelineManager::CreateGraphicsPipeline(const std::vector<VkPipelineShaderStageCreateInfo> &ShaderStages)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan graphics pipeline";
    
    const auto BindingDescription = GetBindingDescriptors();
    const auto AttributeDescriptions = GetAttributeDescriptions();

    const VkPipelineVertexInputStateCreateInfo VertexInputState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<std::uint32_t>(BindingDescription.size()),
        .pVertexBindingDescriptions = BindingDescription.data(),
        .vertexAttributeDescriptionCount = static_cast<std::uint32_t>(AttributeDescriptions.size()),
        .pVertexAttributeDescriptions = AttributeDescriptions.data()};

    const VkPipelineInputAssemblyStateCreateInfo InputAssemblyState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE};

    const VkPipelineViewportStateCreateInfo ViewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1u,
        .pViewports = nullptr,
        .scissorCount = 1u,
        .pScissors = nullptr};

    const VkPipelineRasterizationStateCreateInfo RasterizationState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.f};

    const VkPipelineMultisampleStateCreateInfo MultisampleState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
    };

    const VkPipelineColorBlendAttachmentState ColorBlendAttachmentStates{
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

    const VkPipelineColorBlendStateCreateInfo ColorBlendState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1u,
        .pAttachments = &ColorBlendAttachmentStates,
        .blendConstants = {0.f, 0.f, 0.f, 0.f}};
        
    const VkPipelineDynamicStateCreateInfo DynamicState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(g_DynamicStates.size()),
        .pDynamicStates = g_DynamicStates.data()};

    const VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1u,
        .pSetLayouts = &m_DescriptorSetLayout};

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreatePipelineLayout(m_Device, &PipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));
    
    const VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<std::uint32_t>(ShaderStages.size()),
        .pStages = ShaderStages.data(),
        .pVertexInputState = &VertexInputState,
        .pInputAssemblyState = &InputAssemblyState,
        .pViewportState = &ViewportState,
        .pRasterizationState = &RasterizationState,
        .pMultisampleState = &MultisampleState,
        .pColorBlendState = &ColorBlendState,
        .pDynamicState = &DynamicState,
        .layout = m_PipelineLayout,
        .renderPass = m_RenderPass,
        .subpass = 0u,
        .basePipelineHandle = VK_NULL_HANDLE};

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1u, &GraphicsPipelineCreateInfo, nullptr, &m_Pipeline));
}

void VulkanPipelineManager::CreateDescriptorSetLayout()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan decriptor set layout";

    const VkDescriptorSetLayoutBinding LayoutBinding{
        .binding = 0u,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1u,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr};

    const VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1u,
        .pBindings = &LayoutBinding};

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateDescriptorSetLayout(m_Device, &DescriptorSetLayoutInfo, nullptr, &m_DescriptorSetLayout));
}

void VulkanPipelineManager::CreateDescriptorPool()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan descriptor pool";

    const VkDescriptorPoolSize DescriptorPoolSize{
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = g_MaxFramesInFlight};

    const VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = g_MaxFramesInFlight,
        .poolSizeCount = 1u,
        .pPoolSizes = &DescriptorPoolSize};

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateDescriptorPool(m_Device, &DescriptorPoolCreateInfo, nullptr, &m_DescriptorPool));
}

void VulkanPipelineManager::CreateDescriptorSets(const std::vector<VkBuffer> &UniformBuffers)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan descriptor sets";

    const std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts(g_MaxFramesInFlight, m_DescriptorSetLayout);
    const VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_DescriptorPool,
        .descriptorSetCount = static_cast<std::uint32_t>(m_DescriptorSetLayouts.size()),
        .pSetLayouts = m_DescriptorSetLayouts.data()};
        
    m_DescriptorSets.resize(g_MaxFramesInFlight);
    RENDERCORE_CHECK_VULKAN_RESULT(vkAllocateDescriptorSets(m_Device, &DescriptorSetAllocateInfo, m_DescriptorSets.data()));

    std::vector<VkWriteDescriptorSet> WriteDescriptorSets(g_MaxFramesInFlight);

    for (auto Iterator = WriteDescriptorSets.begin(); Iterator != WriteDescriptorSets.end(); ++Iterator)
    {
        const std::uint32_t Index = std::distance(WriteDescriptorSets.begin(), Iterator);              

        const VkDescriptorBufferInfo DescriptorBufferInfo{
            .buffer = UniformBuffers[Index],
            .offset = 0u,
            .range = sizeof(UniformBufferObject)};

        Iterator->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        Iterator->dstSet = m_DescriptorSets[Index];
        Iterator->dstBinding = 0u;
        Iterator->dstArrayElement = 0u;
        Iterator->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        Iterator->descriptorCount = 1u;
        Iterator->pBufferInfo = &DescriptorBufferInfo;
        Iterator->pImageInfo = nullptr;
        Iterator->pTexelBufferView = nullptr;
    }
    
    vkUpdateDescriptorSets(m_Device, static_cast<std::uint32_t>(WriteDescriptorSets.size()), WriteDescriptorSets.data(), 0u, nullptr);
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

    vkDestroyPipelineCache(m_Device, m_PipelineCache, nullptr);
    m_PipelineCache = VK_NULL_HANDLE;

    vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
    m_DescriptorSetLayout = VK_NULL_HANDLE;

    vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
    m_DescriptorPool = VK_NULL_HANDLE;
    
    m_DescriptorSets.clear();
}

bool VulkanPipelineManager::IsInitialized() const
{
    return m_RenderPass != VK_NULL_HANDLE && m_Pipeline != VK_NULL_HANDLE && m_PipelineLayout != VK_NULL_HANDLE;
}

const VkRenderPass &VulkanPipelineManager::GetRenderPass() const
{
    return m_RenderPass;
}

const VkPipeline &VulkanPipelineManager::GetPipeline() const
{
    return m_Pipeline;
}

const VkPipelineLayout &VulkanPipelineManager::GetPipelineLayout() const
{
    return m_PipelineLayout;
}

const VkPipelineCache &VulkanPipelineManager::GetPipelineCache() const
{
    return m_PipelineCache;
}

const VkDescriptorSetLayout &VulkanPipelineManager::GetDescriptorSetLayout() const
{
    return m_DescriptorSetLayout;
}

const VkDescriptorPool &VulkanPipelineManager::GetDescriptorPool() const
{
    return m_DescriptorPool;
}
const std::vector<VkDescriptorSet> &VulkanPipelineManager::GetDescriptorSets() const
{
    return m_DescriptorSets;
}
