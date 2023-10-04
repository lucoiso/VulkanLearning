// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <boost/log/trivial.hpp>
#include <volk.h>

module RenderCore.Managers.PipelineManager;

import <array>;
import <vector>;

import RenderCore.Managers.DeviceManager;
import RenderCore.Managers.BufferManager;
import RenderCore.Managers.ShaderManager;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import RenderCore.Types.DeviceProperties;
import RenderCore.Types.TextureData;

using namespace RenderCore;

PipelineManager::PipelineManager()
    : m_RenderPass(VK_NULL_HANDLE),
      m_Pipeline(VK_NULL_HANDLE),
      m_PipelineLayout(VK_NULL_HANDLE),
      m_PipelineCache(VK_NULL_HANDLE),
      m_DescriptorPool(VK_NULL_HANDLE),
      m_DescriptorSetLayout(VK_NULL_HANDLE),
      m_DescriptorSets({})
{
}

PipelineManager::~PipelineManager()
{
    try
    {
        Shutdown();
    }
    catch (...)
    {
    }
}

PipelineManager& PipelineManager::Get()
{
    static PipelineManager Instance {};
    return Instance;
}

void PipelineManager::CreateRenderPass()
{
    if (m_RenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(DeviceManager::Get().GetLogicalDevice(), m_RenderPass, nullptr);
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan render pass";

    auto const& [Format, DepthFormat, Mode, Extent, Capabilities] = DeviceManager::Get().GetDeviceProperties();

    VkAttachmentDescription const ColorAttachmentDescription {
            .format         = Format.format,
            .samples        = g_MSAASamples,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};

    constexpr VkAttachmentReference ColorAttachmentReference {
            .attachment = 0U,
            .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkAttachmentDescription const DepthAttachmentDescription {
            .format         = DepthFormat,
            .samples        = g_MSAASamples,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    constexpr VkAttachmentReference DepthAttachmentReference {
            .attachment = 1U,
            .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription const SubpassDescription {
            .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount    = 1U,
            .pColorAttachments       = &ColorAttachmentReference,
            .pDepthStencilAttachment = &DepthAttachmentReference};

    constexpr VkSubpassDependency SubpassDependency {
            .srcSubpass    = VK_SUBPASS_EXTERNAL,
            .dstSubpass    = 0U,
            .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = 0U,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};

    std::array const AttachmentDescriptions {
            ColorAttachmentDescription,
            DepthAttachmentDescription};

    VkRenderPassCreateInfo const RenderPassCreateInfo {
            .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = static_cast<std::uint32_t>(AttachmentDescriptions.size()),
            .pAttachments    = AttachmentDescriptions.data(),
            .subpassCount    = 1U,
            .pSubpasses      = &SubpassDescription,
            .dependencyCount = 1U,
            .pDependencies   = &SubpassDependency};

    Helpers::CheckVulkanResult(vkCreateRenderPass(DeviceManager::Get().GetLogicalDevice(), &RenderPassCreateInfo, nullptr, &m_RenderPass));
}

void PipelineManager::CreateDefaultRenderPass()
{
    constexpr VkRenderPassCreateInfo RenderPassCreateInfo {
            .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 0U,
            .pAttachments    = nullptr,
            .subpassCount    = 0U,
            .pSubpasses      = nullptr,
            .dependencyCount = 0U,
            .pDependencies   = nullptr};

    Helpers::CheckVulkanResult(vkCreateRenderPass(DeviceManager::Get().GetLogicalDevice(), &RenderPassCreateInfo, nullptr, &m_RenderPass));
}

void PipelineManager::CreateGraphicsPipeline()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan graphics pipeline";

    auto const BindingDescription    = Helpers::GetBindingDescriptors();
    auto const AttributeDescriptions = Helpers::GetAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo const VertexInputState {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = static_cast<std::uint32_t>(BindingDescription.size()),
            .pVertexBindingDescriptions      = BindingDescription.data(),
            .vertexAttributeDescriptionCount = static_cast<std::uint32_t>(AttributeDescriptions.size()),
            .pVertexAttributeDescriptions    = AttributeDescriptions.data()};

    constexpr VkPipelineInputAssemblyStateCreateInfo InputAssemblyState {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE};

    constexpr VkPipelineViewportStateCreateInfo ViewportState {
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1U,
            .pViewports    = nullptr,
            .scissorCount  = 1U,
            .pScissors     = nullptr};

    constexpr VkPipelineRasterizationStateCreateInfo RasterizationState {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable        = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode             = VK_POLYGON_MODE_FILL,
            .cullMode                = VK_CULL_MODE_BACK_BIT,
            .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable         = VK_FALSE,
            .depthBiasConstantFactor = 0.F,
            .depthBiasClamp          = 0.F,
            .depthBiasSlopeFactor    = 0.F,
            .lineWidth               = 1.F};

    constexpr VkPipelineMultisampleStateCreateInfo MultisampleState {
            .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples  = g_MSAASamples,
            .sampleShadingEnable   = VK_FALSE,
            .minSampleShading      = 1.F,
            .pSampleMask           = nullptr,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable      = VK_FALSE};

    constexpr VkPipelineColorBlendAttachmentState ColorBlendAttachmentStates {
            .blendEnable         = VK_FALSE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
            .colorBlendOp        = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp        = VK_BLEND_OP_ADD,
            .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

    VkPipelineColorBlendStateCreateInfo const ColorBlendState {
            .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable   = VK_FALSE,
            .logicOp         = VK_LOGIC_OP_COPY,
            .attachmentCount = 1U,
            .pAttachments    = &ColorBlendAttachmentStates,
            .blendConstants  = {
                    0.F,
                    0.F,
                    0.F,
                    0.F}};

    constexpr VkPipelineDynamicStateCreateInfo DynamicState {
            .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = static_cast<uint32_t>(g_DynamicStates.size()),
            .pDynamicStates    = g_DynamicStates.data()};

    constexpr VkPushConstantRange PushConstantRange {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset     = 0U,
            .size       = sizeof(UniformBufferObject)};

    VkPipelineLayoutCreateInfo const PipelineLayoutCreateInfo {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount         = 1U,
            .pSetLayouts            = &m_DescriptorSetLayout,
            .pushConstantRangeCount = 1U,
            .pPushConstantRanges    = &PushConstantRange};

    VkDevice const& VulkanLogicalDevice = DeviceManager::Get().GetLogicalDevice();

    Helpers::CheckVulkanResult(vkCreatePipelineLayout(VulkanLogicalDevice, &PipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));

    constexpr VkPipelineCacheCreateInfo PipelineCacheCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};

    Helpers::CheckVulkanResult(vkCreatePipelineCache(VulkanLogicalDevice, &PipelineCacheCreateInfo, nullptr, &m_PipelineCache));

    constexpr VkPipelineDepthStencilStateCreateInfo DepthStencilState {
            .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable       = VK_TRUE,
            .depthWriteEnable      = VK_TRUE,
            .depthCompareOp        = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable     = VK_FALSE,
            .front                 = VkStencilOpState {},
            .back                  = VkStencilOpState {},
            .minDepthBounds        = 0.F,
            .maxDepthBounds        = 1.F};

    std::vector<VkPipelineShaderStageCreateInfo> const ShaderStages = ShaderManager::Get().GetStageInfos();

    VkGraphicsPipelineCreateInfo const GraphicsPipelineCreateInfo {
            .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount          = static_cast<std::uint32_t>(ShaderStages.size()),
            .pStages             = ShaderStages.data(),
            .pVertexInputState   = &VertexInputState,
            .pInputAssemblyState = &InputAssemblyState,
            .pViewportState      = &ViewportState,
            .pRasterizationState = &RasterizationState,
            .pMultisampleState   = &MultisampleState,
            .pDepthStencilState  = &DepthStencilState,
            .pColorBlendState    = &ColorBlendState,
            .pDynamicState       = &DynamicState,
            .layout              = m_PipelineLayout,
            .renderPass          = m_RenderPass,
            .subpass             = 0U,
            .basePipelineHandle  = VK_NULL_HANDLE,
            .basePipelineIndex   = -1};

    Helpers::CheckVulkanResult(vkCreateGraphicsPipelines(VulkanLogicalDevice, VK_NULL_HANDLE, 1U, &GraphicsPipelineCreateInfo, nullptr, &m_Pipeline));
}

void PipelineManager::CreateDescriptorSetLayout()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan decriptor set layout";

    std::vector const LayoutBindings {
            VkDescriptorSetLayoutBinding {
                    .binding            = 0U,
                    .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount    = 1U,
                    .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
                    .pImmutableSamplers = nullptr},
            VkDescriptorSetLayoutBinding {
                    .binding            = 1U,
                    .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount    = 1U,
                    .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr}};

    VkDescriptorSetLayoutCreateInfo const DescriptorSetLayoutInfo {
            .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<std::uint32_t>(LayoutBindings.size()),
            .pBindings    = LayoutBindings.data()};

    Helpers::CheckVulkanResult(vkCreateDescriptorSetLayout(DeviceManager::Get().GetLogicalDevice(), &DescriptorSetLayoutInfo, nullptr, &m_DescriptorSetLayout));
}

void PipelineManager::CreateDescriptorPool()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan descriptor pool";

    std::vector const DescriptorPoolSizes {
            VkDescriptorPoolSize {
                    .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = g_MaxFramesInFlight},
            VkDescriptorPoolSize {
                    .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = g_MaxFramesInFlight}};

    VkDescriptorPoolCreateInfo const DescriptorPoolCreateInfo {
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets       = g_MaxFramesInFlight,
            .poolSizeCount = static_cast<std::uint32_t>(DescriptorPoolSizes.size()),
            .pPoolSizes    = DescriptorPoolSizes.data()};

    Helpers::CheckVulkanResult(vkCreateDescriptorPool(DeviceManager::Get().GetLogicalDevice(), &DescriptorPoolCreateInfo, nullptr, &m_DescriptorPool));
}

void PipelineManager::CreateDescriptorSets()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan descriptor sets";

    VkDevice const& VulkanLogicalDevice = DeviceManager::Get().GetLogicalDevice();

    std::vector const DescriptorSetLayouts(g_MaxFramesInFlight, m_DescriptorSetLayout);

    VkDescriptorSetAllocateInfo const DescriptorSetAllocateInfo {
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool     = m_DescriptorPool,
            .descriptorSetCount = static_cast<std::uint32_t>(DescriptorSetLayouts.size()),
            .pSetLayouts        = DescriptorSetLayouts.data()};

    m_DescriptorSets.resize(g_MaxFramesInFlight);
    Helpers::CheckVulkanResult(vkAllocateDescriptorSets(VulkanLogicalDevice, &DescriptorSetAllocateInfo, m_DescriptorSets.data()));

    for (std::uint32_t Iterator = 0U; Iterator < g_MaxFramesInFlight; ++Iterator)
    {
        std::vector<VkDescriptorImageInfo> ImageInfos {};
        for (VulkanTextureData const& TextureDataIter: BufferManager::Get().GetAllocatedTextures())
        {
            ImageInfos.push_back(
                    VkDescriptorImageInfo {
                            .sampler     = TextureDataIter.Sampler,
                            .imageView   = TextureDataIter.ImageView,
                            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
        }

        VulkanTextureData const ImGuiTextureAllocation = BufferManager::Get().GetAllocatedImGuiFontTexture();

        ImageInfos.push_back(
                VkDescriptorImageInfo {
                        .sampler     = ImGuiTextureAllocation.Sampler,
                        .imageView   = ImGuiTextureAllocation.ImageView,
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});

        std::vector<VkWriteDescriptorSet> WriteDescriptors {};

        if (!ImageInfos.empty())
        {
            WriteDescriptors.push_back(
                    VkWriteDescriptorSet {
                            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                            .dstSet           = m_DescriptorSets[Iterator],
                            .dstBinding       = 1U,
                            .dstArrayElement  = 0U,
                            .descriptorCount  = static_cast<std::uint32_t>(ImageInfos.size()),
                            .descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            .pImageInfo       = ImageInfos.data(),
                            .pBufferInfo      = nullptr,
                            .pTexelBufferView = nullptr});
        }

        if (!WriteDescriptors.empty())
        {
            vkUpdateDescriptorSets(VulkanLogicalDevice, static_cast<std::uint32_t>(WriteDescriptors.size()), WriteDescriptors.data(), 0U, nullptr);
        }
    }
}

void PipelineManager::Shutdown()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Shutting down vulkan pipelines";

    DestroyResources();
}

void PipelineManager::DestroyResources()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Destroying vulkan pipelines resources";

    VkDevice const& VulkanLogicalDevice = DeviceManager::Get().GetLogicalDevice();

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

VkRenderPass const& PipelineManager::GetRenderPass() const
{
    return m_RenderPass;
}

VkPipeline const& PipelineManager::GetPipeline() const
{
    return m_Pipeline;
}

VkPipelineLayout const& PipelineManager::GetPipelineLayout() const
{
    return m_PipelineLayout;
}

VkPipelineCache const& PipelineManager::GetPipelineCache() const
{
    return m_PipelineCache;
}

VkDescriptorSetLayout const& PipelineManager::GetDescriptorSetLayout() const
{
    return m_DescriptorSetLayout;
}

VkDescriptorPool const& PipelineManager::GetDescriptorPool() const
{
    return m_DescriptorPool;
}

std::vector<VkDescriptorSet> const& PipelineManager::GetDescriptorSets() const
{
    return m_DescriptorSets;
}