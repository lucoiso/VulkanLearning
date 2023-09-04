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
    , m_DescriptorSetLayout(VK_NULL_HANDLE)
    , m_DescriptorPool(VK_NULL_HANDLE)
    , m_DescriptorSets({})
    , m_Viewports{
        VkViewport{
            .x = 0.f,
            .y = 0.f,
            .width = 0.f,
            .height = 0.f,
            .minDepth = 0.f,
            .maxDepth = 1.f}},
      m_Scissors{
        VkRect2D{
            .offset = {0, 0}
            , .extent = {0, 0}}}
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

    const std::vector<VkAttachmentDescription> ColorAttachmentDescriptions{
        VkAttachmentDescription{
            .format = Format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR}};

    const std::vector<VkAttachmentReference> ColorAttachmentReferences{
        VkAttachmentReference{
            .attachment = 0u,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};

    const std::vector<VkSubpassDescription> SubpassDescriptions{
        VkSubpassDescription{
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = static_cast<std::uint32_t>(ColorAttachmentReferences.size()),
            .pColorAttachments = ColorAttachmentReferences.data()}};

    const std::vector<VkSubpassDependency> SubpassDependencies{
        VkSubpassDependency{
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0u,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0u,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT}};

    const VkRenderPassCreateInfo RenderPassCreateInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<std::uint32_t>(ColorAttachmentDescriptions.size()),
        .pAttachments = ColorAttachmentDescriptions.data(),
        .subpassCount = static_cast<std::uint32_t>(SubpassDescriptions.size()),
        .pSubpasses = SubpassDescriptions.data(),
        .dependencyCount = static_cast<std::uint32_t>(SubpassDependencies.size()),
        .pDependencies = SubpassDependencies.data()};

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateRenderPass(m_Device, &RenderPassCreateInfo, nullptr, &m_RenderPass));
}

void VulkanPipelineManager::CreateGraphicsPipeline(const std::vector<VkPipelineShaderStageCreateInfo> &ShaderStages, const VkExtent2D &Extent)
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan graphics pipeline";
    
    UpdateExtent(Extent);
    
    const VkPipelineDynamicStateCreateInfo DynamicState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(g_DynamicStates.size()),
        .pDynamicStates = g_DynamicStates.data()};

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

    const VkPipelineRasterizationStateCreateInfo RasterizationState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.f};

    const std::vector<VkPipelineColorBlendAttachmentState> ColorBlendAttachmentStates{
        VkPipelineColorBlendAttachmentState{
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT}};

    const VkPipelineColorBlendStateCreateInfo ColorBlendState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = static_cast<std::uint32_t>(ColorBlendAttachmentStates.size()),
        .pAttachments = ColorBlendAttachmentStates.data(),
        .blendConstants = {0.f, 0.f, 0.f, 0.f}};

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
        .stencilTestEnable = VK_FALSE};

    const VkPipelineViewportStateCreateInfo ViewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = static_cast<std::uint32_t>(m_Viewports.size()),
        .pViewports = m_Viewports.data(),
        .scissorCount = static_cast<std::uint32_t>(m_Scissors.size()),
        .pScissors = m_Scissors.data()};

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
            .pDynamicState = &DynamicState,
            .layout = m_PipelineLayout,
            .renderPass = m_RenderPass,
            .subpass = 0u}};

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateGraphicsPipelines(m_Device, m_PipelineCache, static_cast<std::uint32_t>(GraphicsPipelineCreateInfo.size()), GraphicsPipelineCreateInfo.data(), nullptr, &m_Pipeline));
}

void VulkanPipelineManager::CreateDescriptorsAndPipelineCache(const std::vector<VkBuffer> &UniformBuffers)
{
    const VkDescriptorPoolSize DescriptorPoolSize{
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = g_MaxFramesInFlight};

    const VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = g_MaxFramesInFlight,
        .poolSizeCount = 1u,
        .pPoolSizes = &DescriptorPoolSize};

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateDescriptorPool(m_Device, &DescriptorPoolCreateInfo, nullptr, &m_DescriptorPool));

    const VkPipelineCacheCreateInfo PipelineCacheCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};

    const std::vector<VkDescriptorSetLayoutBinding> LayoutBinding{
        VkDescriptorSetLayoutBinding{
            .binding = 0u,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1u,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr}};

    const VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<std::uint32_t>(LayoutBinding.size()),
        .pBindings = LayoutBinding.data()};

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateDescriptorSetLayout(m_Device, &DescriptorSetLayoutInfo, nullptr, &m_DescriptorSetLayout));
    const std::vector<VkDescriptorSetLayout> DescriptorSetLayouts(g_MaxFramesInFlight, m_DescriptorSetLayout);

    const VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = g_MaxFramesInFlight,
        .pSetLayouts = DescriptorSetLayouts.data(),
        .pushConstantRangeCount = 0u};

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreatePipelineLayout(m_Device, &PipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));
    RENDERCORE_CHECK_VULKAN_RESULT(vkCreatePipelineCache(m_Device, &PipelineCacheCreateInfo, nullptr, &m_PipelineCache));

    const VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_DescriptorPool,
        .descriptorSetCount = g_MaxFramesInFlight,
        .pSetLayouts = DescriptorSetLayouts.data()};
        
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

void VulkanPipelineManager::UpdateExtent(const VkExtent2D &Extent)
{
    const std::uint32_t NumViewports = static_cast<std::uint32_t>(m_Viewports.size());
    for (VkViewport &ViewportIter : m_Viewports)
    {
        ViewportIter.width = static_cast<float>(Extent.width / NumViewports);
        ViewportIter.height = static_cast<float>(Extent.height / NumViewports);
    }

    const std::uint32_t NumScissors = static_cast<std::uint32_t>(m_Scissors.size());
    for (VkRect2D &ScissorIter : m_Scissors)
    {
        ScissorIter.extent.width = static_cast<float>(Extent.width / NumScissors);
        ScissorIter.extent.height = static_cast<float>(Extent.height / NumScissors);
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

const std::vector<VkViewport> &VulkanPipelineManager::GetViewports() const
{
    return m_Viewports;
}

const std::vector<VkRect2D> &VulkanPipelineManager::GetScissors() const
{
    return m_Scissors;
}
