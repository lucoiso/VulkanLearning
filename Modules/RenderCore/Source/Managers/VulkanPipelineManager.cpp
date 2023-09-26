// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#include "Managers/VulkanPipelineManager.h"
#include "Managers/VulkanDeviceManager.h"
#include "Managers/VulkanShaderManager.h"
#include "Managers/VulkanBufferManager.h"
#include "Utils/RenderCoreHelpers.h"
#include "Types/VulkanVertex.h"
#include "Types/VulkanUniformBufferObject.h"
#include "Utils/RenderCoreHelpers.h"
#include "Utils/VulkanConstants.h"
#include <boost/log/trivial.hpp>

using namespace RenderCore;

VulkanPipelineManager VulkanPipelineManager::g_Instance{};

VulkanPipelineManager::VulkanPipelineManager()
    : m_RenderPass(VK_NULL_HANDLE)
    , m_Pipeline(VK_NULL_HANDLE)
    , m_PipelineLayout(VK_NULL_HANDLE)
    , m_PipelineCache(VK_NULL_HANDLE)
    , m_DescriptorPool(VK_NULL_HANDLE)
    , m_DescriptorSetLayout(VK_NULL_HANDLE)
    , m_DescriptorSets({})
{
}

VulkanPipelineManager::~VulkanPipelineManager()
{
    RenderCoreHelpers::ShutdownManagers();
}

VulkanPipelineManager &VulkanPipelineManager::Get()
{
    return g_Instance;
}

void VulkanPipelineManager::CreateRenderPass()
{
    if (m_RenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(VulkanDeviceManager::Get().GetLogicalDevice(), m_RenderPass, nullptr);
    }
    
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan render pass";

    const VulkanDeviceProperties &DeviceProperties = VulkanDeviceManager::Get().GetDeviceProperties();

    const VkAttachmentDescription ColorAttachmentDescription{
        .format = DeviceProperties.Format.format,
        .samples = g_MSAASamples,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};

    const VkAttachmentReference ColorAttachmentReference{
        .attachment = 0u,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    const VkAttachmentDescription DepthAttachmentDescription{
        .format = DeviceProperties.DepthFormat,
        .samples = g_MSAASamples,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    const VkAttachmentReference DepthAttachmentReference{
        .attachment = 1u,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    const VkSubpassDescription SubpassDescription{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1u,
        .pColorAttachments = &ColorAttachmentReference,
        .pDepthStencilAttachment = &DepthAttachmentReference};

    const VkSubpassDependency SubpassDependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0u,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0u,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};

    const std::array<VkAttachmentDescription, 2u> AttachmentDescriptions{
        ColorAttachmentDescription,
        DepthAttachmentDescription};

    const VkRenderPassCreateInfo RenderPassCreateInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<std::uint32_t>(AttachmentDescriptions.size()),
        .pAttachments = AttachmentDescriptions.data(),
        .subpassCount = 1u,
        .pSubpasses = &SubpassDescription,
        .dependencyCount = 1u,
        .pDependencies = &SubpassDependency};

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateRenderPass(VulkanDeviceManager::Get().GetLogicalDevice(), &RenderPassCreateInfo, nullptr, &m_RenderPass));
}

void VulkanPipelineManager::CreateDefaultRenderPass()
{
    const VkRenderPassCreateInfo RenderPassCreateInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 0u,
        .pAttachments = nullptr,
        .subpassCount = 0u,
        .pSubpasses = nullptr,
        .dependencyCount = 0u,
        .pDependencies = nullptr};

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateRenderPass(VulkanDeviceManager::Get().GetLogicalDevice(), &RenderPassCreateInfo, nullptr, &m_RenderPass));
}

void VulkanPipelineManager::CreateGraphicsPipeline()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan graphics pipeline";
    
    const auto BindingDescription = RenderCoreHelpers::GetBindingDescriptors();
    const auto AttributeDescriptions = RenderCoreHelpers::GetAttributeDescriptions();

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
        .depthBiasConstantFactor = 0.f,
        .depthBiasClamp = 0.f,
        .depthBiasSlopeFactor = 0.f,
        .lineWidth = 1.f};

    const VkPipelineMultisampleStateCreateInfo MultisampleState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = g_MSAASamples,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE};

    const VkPipelineColorBlendAttachmentState ColorBlendAttachmentStates{
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
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

    const VkPushConstantRange PushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0u,
        .size = sizeof(UniformBufferObject)};

    const VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1u,
        .pSetLayouts = &m_DescriptorSetLayout,
        .pushConstantRangeCount = 1u,
        .pPushConstantRanges = &PushConstantRange};

    const VkDevice &VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreatePipelineLayout(VulkanLogicalDevice, &PipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));

    const VkPipelineCacheCreateInfo PipelineCacheCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreatePipelineCache(VulkanLogicalDevice, &PipelineCacheCreateInfo, nullptr, &m_PipelineCache));
    
    const VkPipelineDepthStencilStateCreateInfo DepthStencilState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = VkStencilOpState{},
        .back = VkStencilOpState{},
        .minDepthBounds = 0.f,
        .maxDepthBounds = 1.f};

    const std::vector<VkPipelineShaderStageCreateInfo> ShaderStages = VulkanShaderManager::Get().GetStageInfos();

    const VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo{
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
        .subpass = 0u,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1};

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateGraphicsPipelines(VulkanLogicalDevice, VK_NULL_HANDLE, 1u, &GraphicsPipelineCreateInfo, nullptr, &m_Pipeline));
}

void VulkanPipelineManager::CreateDescriptorSetLayout()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan decriptor set layout";

    const std::vector<VkDescriptorSetLayoutBinding> LayoutBindings{
        VkDescriptorSetLayoutBinding{
            .binding = 0u,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1u,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr},
        VkDescriptorSetLayoutBinding{
            .binding = 1u,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1u,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr}};

    const VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<std::uint32_t>(LayoutBindings.size()),
        .pBindings = LayoutBindings.data()};

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateDescriptorSetLayout(VulkanDeviceManager::Get().GetLogicalDevice(), &DescriptorSetLayoutInfo, nullptr, &m_DescriptorSetLayout));
}

void VulkanPipelineManager::CreateDescriptorPool()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan descriptor pool";

    const std::vector<VkDescriptorPoolSize> DescriptorPoolSizes{
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = g_MaxFramesInFlight},
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = g_MaxFramesInFlight}};

    const VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = g_MaxFramesInFlight,
        .poolSizeCount = static_cast<std::uint32_t>(DescriptorPoolSizes.size()),
        .pPoolSizes = DescriptorPoolSizes.data()};

    RENDERCORE_CHECK_VULKAN_RESULT(vkCreateDescriptorPool(VulkanDeviceManager::Get().GetLogicalDevice(), &DescriptorPoolCreateInfo, nullptr, &m_DescriptorPool));
}

void VulkanPipelineManager::CreateDescriptorSets()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan descriptor sets";
    
    const VkDevice &VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

    const std::vector<VkDescriptorSetLayout> DescriptorSetLayouts(g_MaxFramesInFlight, m_DescriptorSetLayout);
    
    const VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_DescriptorPool,
        .descriptorSetCount = static_cast<std::uint32_t>(DescriptorSetLayouts.size()),
        .pSetLayouts = DescriptorSetLayouts.data()};
        
    m_DescriptorSets.resize(g_MaxFramesInFlight);
    RENDERCORE_CHECK_VULKAN_RESULT(vkAllocateDescriptorSets(VulkanLogicalDevice, &DescriptorSetAllocateInfo, m_DescriptorSets.data()));

    for (std::uint32_t Iterator = 0u; Iterator < g_MaxFramesInFlight; ++Iterator)
    {
        std::vector<VkDescriptorImageInfo> ImageInfos{};
        for (const VulkanTextureData &TextureDataIter : VulkanBufferManager::Get().GetAllocatedTextures())
        {
            ImageInfos.push_back(VkDescriptorImageInfo{
                .sampler = TextureDataIter.Sampler,
                .imageView = TextureDataIter.ImageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
        }

        std::vector<VkWriteDescriptorSet> WriteDescriptors {};

        if (!ImageInfos.empty())
        {
            WriteDescriptors.push_back(VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_DescriptorSets[Iterator],
                .dstBinding = 1u,
                .dstArrayElement = 0u,
                .descriptorCount = static_cast<std::uint32_t>(ImageInfos.size()),
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = ImageInfos.data(),
                .pBufferInfo = nullptr,
                .pTexelBufferView = nullptr});
        }        
    
        if (!WriteDescriptors.empty())
        {
            vkUpdateDescriptorSets(VulkanLogicalDevice, static_cast<std::uint32_t>(WriteDescriptors.size()), WriteDescriptors.data(), 0u, nullptr);
        }
    }
}

void VulkanPipelineManager::Shutdown()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan pipelines";

    DestroyResources();

    const VkDevice &VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

    if (m_Pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(VulkanLogicalDevice, m_Pipeline, nullptr);
        m_Pipeline = VK_NULL_HANDLE;
    }

    if (m_PipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(VulkanLogicalDevice, m_PipelineLayout, nullptr);
        m_PipelineLayout = VK_NULL_HANDLE;
    }

    if (m_PipelineCache != VK_NULL_HANDLE)
    {
        vkDestroyPipelineCache(VulkanLogicalDevice, m_PipelineCache, nullptr);
        m_PipelineCache = VK_NULL_HANDLE;
    }
}

void VulkanPipelineManager::DestroyResources()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying vulkan pipelines resources";

    const VkDevice &VulkanLogicalDevice = VulkanDeviceManager::Get().GetLogicalDevice();

    if (m_RenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(VulkanLogicalDevice, m_RenderPass, nullptr);
        m_RenderPass = VK_NULL_HANDLE;
    }

    if (m_DescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(VulkanLogicalDevice, m_DescriptorSetLayout, nullptr);
        m_DescriptorSetLayout = VK_NULL_HANDLE;
    }

    if (m_DescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(VulkanLogicalDevice, m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }
    
    m_DescriptorSets.clear();
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
