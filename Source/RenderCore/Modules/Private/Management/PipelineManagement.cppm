// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <boost/log/trivial.hpp>
#include <volk.h>

module RenderCore.Management.PipelineManagement;

import <array>;
import <ranges>;

import RenderCore.Management.ShaderManagement;
import RenderCore.Management.DeviceManagement;
import RenderCore.Management.BufferManagement;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Utils.Constants;
import RenderCore.Utils.Helpers;

using namespace RenderCore;

VkRenderPass g_RenderPass {VK_NULL_HANDLE};
VkPipeline g_Pipeline {VK_NULL_HANDLE};
VkPipelineLayout g_PipelineLayout {VK_NULL_HANDLE};
VkPipelineCache g_PipelineCache {VK_NULL_HANDLE};
VkDescriptorPool g_DescriptorPool {VK_NULL_HANDLE};
VkDescriptorSetLayout g_DescriptorSetLayout {VK_NULL_HANDLE};
std::unordered_map<std::uint32_t, VkDescriptorSet> g_DescriptorSets {};

void RenderCore::CreateRenderPass()
{
    if (g_RenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(volkGetLoadedDevice(), g_RenderPass, nullptr);
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
            .attachmentCount = static_cast<std::uint32_t>(std::size(AttachmentDescriptions)),
            .pAttachments    = std::data(AttachmentDescriptions),
            .subpassCount    = 1U,
            .pSubpasses      = &SubpassDescription,
            .dependencyCount = 1U,
            .pDependencies   = &SubpassDependency};

    CheckVulkanResult(vkCreateRenderPass(volkGetLoadedDevice(), &RenderPassCreateInfo, nullptr, &g_RenderPass));
}

void RenderCore::CreateGraphicsPipeline()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan graphics pipeline";

    auto const BindingDescription    = GetBindingDescriptors();
    auto const AttributeDescriptions = GetAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo const VertexInputState {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = static_cast<std::uint32_t>(std::size(BindingDescription)),
            .pVertexBindingDescriptions      = std::data(BindingDescription),
            .vertexAttributeDescriptionCount = static_cast<std::uint32_t>(std::size(AttributeDescriptions)),
            .pVertexAttributeDescriptions    = std::data(AttributeDescriptions)};

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
            .dynamicStateCount = static_cast<uint32_t>(std::size(g_DynamicStates)),
            .pDynamicStates    = std::data(g_DynamicStates)};

    VkPipelineLayoutCreateInfo const PipelineLayoutCreateInfo {
            .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1U,
            .pSetLayouts    = &g_DescriptorSetLayout};

    VkDevice const& VulkanLogicalDevice = volkGetLoadedDevice();

    CheckVulkanResult(vkCreatePipelineLayout(VulkanLogicalDevice, &PipelineLayoutCreateInfo, nullptr, &g_PipelineLayout));

    constexpr VkPipelineCacheCreateInfo PipelineCacheCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};

    CheckVulkanResult(vkCreatePipelineCache(VulkanLogicalDevice, &PipelineCacheCreateInfo, nullptr, &g_PipelineCache));

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
            .stageCount          = static_cast<std::uint32_t>(std::size(ShaderStages)),
            .pStages             = std::data(ShaderStages),
            .pVertexInputState   = &VertexInputState,
            .pInputAssemblyState = &InputAssemblyState,
            .pViewportState      = &ViewportState,
            .pRasterizationState = &RasterizationState,
            .pMultisampleState   = &MultisampleState,
            .pDepthStencilState  = &DepthStencilState,
            .pColorBlendState    = &ColorBlendState,
            .pDynamicState       = &DynamicState,
            .layout              = g_PipelineLayout,
            .renderPass          = g_RenderPass,
            .subpass             = 0U,
            .basePipelineHandle  = VK_NULL_HANDLE,
            .basePipelineIndex   = -1};

    CheckVulkanResult(vkCreateGraphicsPipelines(VulkanLogicalDevice, VK_NULL_HANDLE, 1U, &GraphicsPipelineCreateInfo, nullptr, &g_Pipeline));
}

void RenderCore::CreateDescriptorSetLayout()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan decriptor set layout";

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
            .bindingCount = static_cast<std::uint32_t>(std::size(LayoutBindings)),
            .pBindings    = std::data(LayoutBindings)};

    CheckVulkanResult(vkCreateDescriptorSetLayout(volkGetLoadedDevice(), &DescriptorSetLayoutInfo, nullptr, &g_DescriptorSetLayout));
}

void RenderCore::CreateDescriptorPool()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan descriptor pool";

    std::array const DescriptorPoolSizes {
            VkDescriptorPoolSize {
                    .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = GetClampedNumAllocations()},
            VkDescriptorPoolSize {
                    .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = GetClampedNumAllocations()}};

    VkDescriptorPoolCreateInfo const DescriptorPoolCreateInfo {
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets       = 2U * GetClampedNumAllocations(),
            .poolSizeCount = static_cast<std::uint32_t>(std::size(DescriptorPoolSizes)),
            .pPoolSizes    = std::data(DescriptorPoolSizes)};

    CheckVulkanResult(vkCreateDescriptorPool(volkGetLoadedDevice(), &DescriptorPoolCreateInfo, nullptr, &g_DescriptorPool));
}

void RenderCore::CreateDescriptorSets()
{
    if (GetNumAllocations() == 0U)
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan descriptor sets";

    VkDevice const& VulkanLogicalDevice = volkGetLoadedDevice();
    auto const AllocatedObjects         = GetAllocatedObjects();

    g_DescriptorSets.reserve(GetNumAllocations());

    VkDescriptorSetAllocateInfo const DescriptorSetAllocateInfo {
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool     = g_DescriptorPool,
            .descriptorSetCount = 1U,
            .pSetLayouts        = &g_DescriptorSetLayout};

    for (MeshBufferData const& BufferdataIter: AllocatedObjects)
    {
        VkDescriptorSet AllocatedSet {VK_NULL_HANDLE};
        CheckVulkanResult(vkAllocateDescriptorSets(VulkanLogicalDevice, &DescriptorSetAllocateInfo, &AllocatedSet));

        g_DescriptorSets.emplace(BufferdataIter.ID, AllocatedSet);
    }

    for (auto Iterator = std::begin(AllocatedObjects); Iterator != std::end(AllocatedObjects); ++Iterator)
    {
        constexpr std::size_t UniformBufferSize = sizeof(UniformBufferObject);
        auto const Index                        = static_cast<std::uint32_t>(std::distance(std::begin(AllocatedObjects), Iterator));

        std::vector<VkWriteDescriptorSet> WriteDescriptors {};
        std::vector<VkDescriptorBufferInfo> BufferInfos {};
        std::vector<VkDescriptorImageInfo> ImageInfos {};

        BufferInfos.push_back(VkDescriptorBufferInfo {
                .buffer = AllocatedObjects.at(Index).UniformBuffer,
                .offset = 0U,
                .range  = UniformBufferSize});

        for (auto const& [Type, TextureData]: AllocatedObjects.at(Index).Textures)
        {
            ImageInfos.push_back(VkDescriptorImageInfo {
                    .sampler     = TextureData.Sampler,
                    .imageView   = TextureData.ImageView,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
        }

        if (!std::empty(BufferInfos))
        {
            WriteDescriptors.push_back(VkWriteDescriptorSet {
                    .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet           = g_DescriptorSets.at(Iterator->ID),
                    .dstBinding       = 0U,
                    .dstArrayElement  = 0U,
                    .descriptorCount  = static_cast<std::uint32_t>(std::size(BufferInfos)),
                    .descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pImageInfo       = nullptr,
                    .pBufferInfo      = std::data(BufferInfos),
                    .pTexelBufferView = nullptr});
        }

        if (!std::empty(ImageInfos))
        {
            WriteDescriptors.push_back(VkWriteDescriptorSet {
                    .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet           = g_DescriptorSets.at(Iterator->ID),
                    .dstBinding       = 1U,
                    .dstArrayElement  = 0U,
                    .descriptorCount  = static_cast<std::uint32_t>(std::size(ImageInfos)),
                    .descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo       = std::data(ImageInfos),
                    .pBufferInfo      = nullptr,
                    .pTexelBufferView = nullptr});
        }

        if (!std::empty(WriteDescriptors))
        {
            vkUpdateDescriptorSets(VulkanLogicalDevice, static_cast<std::uint32_t>(std::size(WriteDescriptors)), std::data(WriteDescriptors), 0U, nullptr);
        }
    }
}

void RenderCore::ReleasePipelineResources()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Releasing vulkan pipeline resources";

    VkDevice const& VulkanLogicalDevice = volkGetLoadedDevice();

    if (g_RenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(VulkanLogicalDevice, g_RenderPass, nullptr);
        g_RenderPass = VK_NULL_HANDLE;
    }

    ReleaseDynamicPipelineResources();
}

void RenderCore::ReleaseDynamicPipelineResources()
{
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Releasing vulkan pipeline resources";

    VkDevice const& VulkanLogicalDevice = volkGetLoadedDevice();

    if (g_Pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(VulkanLogicalDevice, g_Pipeline, nullptr);
        g_Pipeline = VK_NULL_HANDLE;
    }

    if (g_PipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(VulkanLogicalDevice, g_PipelineLayout, nullptr);
        g_PipelineLayout = VK_NULL_HANDLE;
    }

    if (g_PipelineCache != VK_NULL_HANDLE)
    {
        vkDestroyPipelineCache(VulkanLogicalDevice, g_PipelineCache, nullptr);
        g_PipelineCache = VK_NULL_HANDLE;
    }

    if (g_DescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(VulkanLogicalDevice, g_DescriptorSetLayout, nullptr);
        g_DescriptorSetLayout = VK_NULL_HANDLE;
    }

    if (g_DescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(VulkanLogicalDevice, g_DescriptorPool, nullptr);
        g_DescriptorPool = VK_NULL_HANDLE;
    }

    g_DescriptorSets.clear();
}

VkRenderPass const& RenderCore::GetRenderPass()
{
    return g_RenderPass;
}

VkPipeline const& RenderCore::GetPipeline()
{
    return g_Pipeline;
}

VkPipelineLayout const& RenderCore::GetPipelineLayout()
{
    return g_PipelineLayout;
}

VkPipelineCache const& RenderCore::GetPipelineCache()
{
    return g_PipelineCache;
}

VkDescriptorSetLayout const& RenderCore::GetDescriptorSetLayout()
{
    return g_DescriptorSetLayout;
}

VkDescriptorPool const& RenderCore::GetDescriptorPool()
{
    return g_DescriptorPool;
}

VkDescriptorSet RenderCore::GetDescriptorSet(std::uint32_t const ObjectID)
{
    if (!g_DescriptorSets.contains(ObjectID))
    {
        return VK_NULL_HANDLE;
    }

    return g_DescriptorSets.at(ObjectID);
}