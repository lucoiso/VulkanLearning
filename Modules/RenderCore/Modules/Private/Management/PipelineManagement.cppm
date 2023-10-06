// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

module;

#include <boost/log/trivial.hpp>
#include <volk.h>

module RenderCore.Management.PipelineManagement;

import <array>;
import <vector>;

import RenderCore.Management.DeviceManagement;
import RenderCore.Management.BufferManagement;
import RenderCore.Management.ShaderManagement;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import RenderCore.Types.DeviceProperties;
import RenderCore.Types.TextureData;

using namespace RenderCore;

static VkRenderPass s_RenderPass {VK_NULL_HANDLE};
static VkPipeline s_Pipeline {VK_NULL_HANDLE};
static VkPipelineLayout s_PipelineLayout {VK_NULL_HANDLE};
static VkPipelineCache s_PipelineCache {VK_NULL_HANDLE};
static VkDescriptorPool s_DescriptorPool {VK_NULL_HANDLE};
static std::array<VkDescriptorSetLayout, 1U> s_DescriptorSetLayouts {};
static std::vector<VkDescriptorSet> s_DescriptorSets {};

void RenderCore::CreateRenderPass()
{
    if (s_RenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(GetLogicalDevice(), s_RenderPass, nullptr);
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan render pass";

    auto const& [Format, DepthFormat, Mode, Extent, Capabilities] = GetDeviceProperties();

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

    CheckVulkanResult(vkCreateRenderPass(GetLogicalDevice(), &RenderPassCreateInfo, nullptr, &s_RenderPass));
}

void RenderCore::CreateGraphicsPipeline()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan graphics pipeline";

    auto const BindingDescription    = GetBindingDescriptors();
    auto const AttributeDescriptions = GetAttributeDescriptions();

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

    constexpr std::uint8_t ImGuiPushConstantSize = sizeof(float) * 4U;

    constexpr std::array PushConstantRanges {
            VkPushConstantRange {
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                    .offset     = 0U,
                    .size       = sizeof(UniformBufferObject) + ImGuiPushConstantSize}};

    VkPipelineLayoutCreateInfo const PipelineLayoutCreateInfo {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount         = static_cast<std::uint32_t>(s_DescriptorSetLayouts.size()),
            .pSetLayouts            = s_DescriptorSetLayouts.data(),
            .pushConstantRangeCount = static_cast<std::uint32_t>(PushConstantRanges.size()),
            .pPushConstantRanges    = PushConstantRanges.data()};

    VkDevice const& VulkanLogicalDevice = GetLogicalDevice();

    CheckVulkanResult(vkCreatePipelineLayout(VulkanLogicalDevice, &PipelineLayoutCreateInfo, nullptr, &s_PipelineLayout));

    constexpr VkPipelineCacheCreateInfo PipelineCacheCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};

    CheckVulkanResult(vkCreatePipelineCache(VulkanLogicalDevice, &PipelineCacheCreateInfo, nullptr, &s_PipelineCache));

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

    std::vector<VkPipelineShaderStageCreateInfo> const ShaderStages = GetStageInfos();

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
            .layout              = s_PipelineLayout,
            .renderPass          = s_RenderPass,
            .subpass             = 0U,
            .basePipelineHandle  = VK_NULL_HANDLE,
            .basePipelineIndex   = -1};

    CheckVulkanResult(vkCreateGraphicsPipelines(VulkanLogicalDevice, VK_NULL_HANDLE, 1U, &GraphicsPipelineCreateInfo, nullptr, &s_Pipeline));
}

void RenderCore::CreateDescriptorSetLayout()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan decriptor set layout";

    // Object Bindings
    {
        constexpr std::array LayoutBindings {
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

        CheckVulkanResult(vkCreateDescriptorSetLayout(GetLogicalDevice(), &DescriptorSetLayoutInfo, nullptr, &s_DescriptorSetLayouts.at(0U)));
    }
}

void RenderCore::CreateDescriptorPool()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan descriptor pool";

    constexpr std::array DescriptorPoolSizes {
            VkDescriptorPoolSize {
                    .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1U},
            VkDescriptorPoolSize {
                    .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 2U}};

    VkDescriptorPoolCreateInfo const DescriptorPoolCreateInfo {
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets       = static_cast<std::uint32_t>(s_DescriptorSetLayouts.size()),
            .poolSizeCount = static_cast<std::uint32_t>(DescriptorPoolSizes.size()),
            .pPoolSizes    = DescriptorPoolSizes.data()};

    CheckVulkanResult(vkCreateDescriptorPool(GetLogicalDevice(), &DescriptorPoolCreateInfo, nullptr, &s_DescriptorPool));
}

void RenderCore::CreateDescriptorSets()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan descriptor sets";

    VkDevice const& VulkanLogicalDevice = GetLogicalDevice();
    auto const AllocatedTextures        = GetAllocatedTextures();

    s_DescriptorSets.resize(s_DescriptorSetLayouts.size());

    VkDescriptorSetAllocateInfo const DescriptorSetAllocateInfo {
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool     = s_DescriptorPool,
            .descriptorSetCount = static_cast<std::uint32_t>(s_DescriptorSets.size()),
            .pSetLayouts        = s_DescriptorSetLayouts.data()};

    CheckVulkanResult(vkAllocateDescriptorSets(VulkanLogicalDevice, &DescriptorSetAllocateInfo, s_DescriptorSets.data()));

    // Object Bindings
    {
        std::vector<VkDescriptorImageInfo> ImageInfos {};
        for (VulkanTextureData const& TextureDataIter: AllocatedTextures)
        {
            ImageInfos.push_back(
                    VkDescriptorImageInfo {
                            .sampler     = TextureDataIter.Sampler,
                            .imageView   = TextureDataIter.ImageView,
                            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
        }

        std::vector<VkWriteDescriptorSet> WriteDescriptors {};

        for (auto ImageInfoIterator = ImageInfos.begin(); ImageInfoIterator != ImageInfos.end(); ++ImageInfoIterator)
        {
            WriteDescriptors.push_back(
                    VkWriteDescriptorSet {
                            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                            .dstSet           = s_DescriptorSets.at(0U),
                            .dstBinding       = 1U,
                            .dstArrayElement  = 0U,
                            .descriptorCount  = 1U,
                            .descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            .pImageInfo       = &(*ImageInfoIterator),
                            .pBufferInfo      = nullptr,
                            .pTexelBufferView = nullptr});
        }

        vkUpdateDescriptorSets(VulkanLogicalDevice, static_cast<std::uint32_t>(WriteDescriptors.size()), WriteDescriptors.data(), 0U, nullptr);
    }
}

void RenderCore::ReleasePipelineResources()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Releasing vulkan pipeline resources";

    VkDevice const& VulkanLogicalDevice = GetLogicalDevice();

    if (s_Pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(VulkanLogicalDevice, s_Pipeline, nullptr);
        s_Pipeline = VK_NULL_HANDLE;
    }

    if (s_PipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(VulkanLogicalDevice, s_PipelineLayout, nullptr);
        s_PipelineLayout = VK_NULL_HANDLE;
    }

    if (s_PipelineCache != VK_NULL_HANDLE)
    {
        vkDestroyPipelineCache(VulkanLogicalDevice, s_PipelineCache, nullptr);
        s_PipelineCache = VK_NULL_HANDLE;
    }

    if (s_RenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(VulkanLogicalDevice, s_RenderPass, nullptr);
        s_RenderPass = VK_NULL_HANDLE;
    }

    for (auto& DescriptorSetLayoutIter: s_DescriptorSetLayouts)
    {
        if (DescriptorSetLayoutIter != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(VulkanLogicalDevice, DescriptorSetLayoutIter, nullptr);
            DescriptorSetLayoutIter = VK_NULL_HANDLE;
        }
    }

    if (s_DescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(VulkanLogicalDevice, s_DescriptorPool, nullptr);
        s_DescriptorPool = VK_NULL_HANDLE;
    }

    s_DescriptorSets.clear();
}

VkRenderPass const& RenderCore::GetRenderPass()
{
    return s_RenderPass;
}

VkPipeline const& RenderCore::GetPipeline()
{
    return s_Pipeline;
}

VkPipelineLayout const& RenderCore::GetPipelineLayout()
{
    return s_PipelineLayout;
}

std::vector<VkDescriptorSet> const& RenderCore::GetDescriptorSets()
{
    return s_DescriptorSets;
}