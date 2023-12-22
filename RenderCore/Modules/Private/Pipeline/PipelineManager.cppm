// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <array>
#include <boost/log/trivial.hpp>
#include <ranges>
#include <volk.h>

module RenderCore.Management.PipelineManagement;

import RenderCore.Management.ShaderManagement;
import RenderCore.Management.DeviceManagement;
import RenderCore.Management.BufferManagement;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Utils.Constants;
import RenderCore.Utils.Helpers;
import Timer.ExecutionCounter;

using namespace RenderCore;

void PipelineManager::CreateRenderPass(SurfaceProperties const& SurfaceProperties, VkImageLayout const FinalLayout, VkRenderPass& OutRenderPass)
{
    if (OutRenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(volkGetLoadedDevice(), OutRenderPass, nullptr);
    }

    std::array const AttachmentDescriptions {
            VkAttachmentDescription {
                    .format         = SurfaceProperties.Format.format,
                    .samples        = g_MSAASamples,
                    .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                    .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
                    .finalLayout    = FinalLayout},
            VkAttachmentDescription {
                    .format         = SurfaceProperties.DepthFormat,
                    .samples        = g_MSAASamples,
                    .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                    .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR,
                    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
                    .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL}};

    constexpr VkAttachmentReference ColorAttachmentReference {
            .attachment = 0U,
            .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    constexpr VkAttachmentReference DepthAttachmentReference {
            .attachment = 1U,
            .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription const SubpassDescription {
            .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount    = 1U,
            .pColorAttachments       = &ColorAttachmentReference,
            .pDepthStencilAttachment = &DepthAttachmentReference};

    constexpr std::array SubpassDependencies {
            VkSubpassDependency {
                    .srcSubpass      = VK_SUBPASS_EXTERNAL,
                    .dstSubpass      = 0U,
                    .srcStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                    .dstStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                    .srcAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                    .dstAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                    .dependencyFlags = 0U},
            VkSubpassDependency {
                    .srcSubpass      = VK_SUBPASS_EXTERNAL,
                    .dstSubpass      = 0U,
                    .srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .srcAccessMask   = 0U,
                    .dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
                    .dependencyFlags = 0U}};

    VkRenderPassCreateInfo const RenderPassCreateInfo {
            .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = static_cast<std::uint32_t>(std::size(AttachmentDescriptions)),
            .pAttachments    = std::data(AttachmentDescriptions),
            .subpassCount    = 1U,
            .pSubpasses      = &SubpassDescription,
            .dependencyCount = static_cast<std::uint32_t>(std::size(SubpassDependencies)),
            .pDependencies   = std::data(SubpassDependencies)};

    CheckVulkanResult(vkCreateRenderPass(volkGetLoadedDevice(), &RenderPassCreateInfo, nullptr, &OutRenderPass));
}

void PipelineManager::CreateRenderPasses(SurfaceProperties const& SurfaceProperties)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan render passes";

    CreateRenderPass(SurfaceProperties, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_MainRenderPass);
    CreateRenderPass(SurfaceProperties, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_ViewportRenderPass);
}

void PipelineManager::CreatePipeline(VkRenderPass const& RenderPass, VkPipelineLayout const& PipelineLayout, VkExtent2D const& ViewportExtent, VkPipeline& OutPipeline)
{
    static auto const BindingDescription    = GetBindingDescriptors();
    static auto const AttributeDescriptions = GetAttributeDescriptions();

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

    VkViewport const Viewport {
            .x        = 0.F,
            .y        = 0.F,
            .width    = static_cast<float>(ViewportExtent.width),
            .height   = static_cast<float>(ViewportExtent.height),
            .minDepth = 0.F,
            .maxDepth = 1.F};

    VkRect2D const Scissor {
            .offset = {0, 0},
            .extent = ViewportExtent};

    VkPipelineViewportStateCreateInfo const ViewportState {
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1U,
            .pViewports    = &Viewport,
            .scissorCount  = 1U,
            .pScissors     = &Scissor};

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
            .blendEnable         = VK_TRUE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
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

    constexpr VkPipelineDepthStencilStateCreateInfo DepthStencilState {
            .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable       = VK_TRUE,
            .depthWriteEnable      = VK_TRUE,
            .depthCompareOp        = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable     = VK_FALSE,
            .front                 = {},
            .back                  = {},
            .minDepthBounds        = 0.F,
            .maxDepthBounds        = 1.F};

    static std::vector<VkPipelineShaderStageCreateInfo> const ShaderStages = GetStageInfos();

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
            .layout              = PipelineLayout,
            .renderPass          = RenderPass,
            .subpass             = 0U,
            .basePipelineHandle  = VK_NULL_HANDLE,
            .basePipelineIndex   = -1};

    CheckVulkanResult(vkCreateGraphicsPipelines(volkGetLoadedDevice(), VK_NULL_HANDLE, 1U, &GraphicsPipelineCreateInfo, nullptr, &OutPipeline));
}

void PipelineManager::CreatePipelines(VkExtent2D const& ViewportExtent)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);
    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan graphics pipelines";

    VkPipelineLayoutCreateInfo const PipelineLayoutCreateInfo {
            .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1U,
            .pSetLayouts    = &m_DescriptorSetLayout};

    CheckVulkanResult(vkCreatePipelineLayout(volkGetLoadedDevice(), &PipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));

    constexpr VkPipelineCacheCreateInfo PipelineCacheCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};

    CheckVulkanResult(vkCreatePipelineCache(volkGetLoadedDevice(), &PipelineCacheCreateInfo, nullptr, &m_PipelineCache));

    CreatePipeline(m_MainRenderPass, m_PipelineLayout, ViewportExtent, m_MainPipeline);
    CreatePipeline(m_ViewportRenderPass, m_PipelineLayout, ViewportExtent, m_ViewportPipeline);
}

void PipelineManager::CreateDescriptorSetLayout()
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

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

    CheckVulkanResult(vkCreateDescriptorSetLayout(volkGetLoadedDevice(), &DescriptorSetLayoutInfo, nullptr, &m_DescriptorSetLayout));
}

void PipelineManager::CreateDescriptorPool(std::uint32_t const NumAllocations)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan descriptor pool";

    std::array const DescriptorPoolSizes {
            VkDescriptorPoolSize {
                    .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = NumAllocations},
            VkDescriptorPoolSize {
                    .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = NumAllocations}};

    VkDescriptorPoolCreateInfo const DescriptorPoolCreateInfo {
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets       = 2U * NumAllocations,
            .poolSizeCount = static_cast<std::uint32_t>(std::size(DescriptorPoolSizes)),
            .pPoolSizes    = std::data(DescriptorPoolSizes)};

    CheckVulkanResult(vkCreateDescriptorPool(volkGetLoadedDevice(), &DescriptorPoolCreateInfo, nullptr, &m_DescriptorPool));
}

void PipelineManager::CreateDescriptorSets(std::vector<MeshBufferData> const& AllocatedObjects, VkSampler const& Sampler)
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    auto const NumAllocations {static_cast<std::uint32_t>(std::size(AllocatedObjects))};

    if (NumAllocations == 0U)
    {
        return;
    }

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Creating vulkan descriptor sets";

    m_DescriptorSets.reserve(NumAllocations);

    VkDescriptorSetAllocateInfo const DescriptorSetAllocateInfo {
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool     = m_DescriptorPool,
            .descriptorSetCount = 1U,
            .pSetLayouts        = &m_DescriptorSetLayout};

    for (auto const& [ID, UniformBuffer, UniformBufferData, Textures]: AllocatedObjects)
    {
        VkDescriptorSet AllocatedSet {VK_NULL_HANDLE};
        CheckVulkanResult(vkAllocateDescriptorSets(volkGetLoadedDevice(), &DescriptorSetAllocateInfo, &AllocatedSet));

        m_DescriptorSets.emplace(ID, AllocatedSet);
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

        for (auto const& View: AllocatedObjects.at(Index).Textures | std::views::values)
        {
            ImageInfos.push_back(VkDescriptorImageInfo {
                    .sampler     = Sampler,
                    .imageView   = View,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
        }

        if (!std::empty(BufferInfos))
        {
            WriteDescriptors.push_back(VkWriteDescriptorSet {
                    .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet          = m_DescriptorSets.at(Iterator->ID),
                    .dstBinding      = 0U,
                    .dstArrayElement = 0U,
                    .descriptorCount = static_cast<std::uint32_t>(std::size(BufferInfos)),
                    .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo     = std::data(BufferInfos)});
        }

        if (!std::empty(ImageInfos))
        {
            WriteDescriptors.push_back(VkWriteDescriptorSet {
                    .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet          = m_DescriptorSets.at(Iterator->ID),
                    .dstBinding      = 1U,
                    .dstArrayElement = 0U,
                    .descriptorCount = static_cast<std::uint32_t>(std::size(ImageInfos)),
                    .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo      = std::data(ImageInfos)});
        }

        if (!std::empty(WriteDescriptors))
        {
            vkUpdateDescriptorSets(volkGetLoadedDevice(), static_cast<std::uint32_t>(std::size(WriteDescriptors)), std::data(WriteDescriptors), 0U, nullptr);
        }
    }
}

void PipelineManager::ReleasePipelineResources()
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Releasing vulkan pipeline resources";

    if (m_MainRenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(volkGetLoadedDevice(), m_MainRenderPass, nullptr);
        m_MainRenderPass = VK_NULL_HANDLE;
    }

    if (m_ViewportRenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(volkGetLoadedDevice(), m_ViewportRenderPass, nullptr);
        m_ViewportRenderPass = VK_NULL_HANDLE;
    }

    ReleaseDynamicPipelineResources();
}

void PipelineManager::ReleaseDynamicPipelineResources()
{
    Timer::ScopedTimer const ScopedExecutionTimer(__func__);

    BOOST_LOG_TRIVIAL(debug) << "[" << __func__ << "]: Releasing vulkan pipeline resources";

    if (m_MainPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(volkGetLoadedDevice(), m_MainPipeline, nullptr);
        m_MainPipeline = VK_NULL_HANDLE;
    }

    if (m_ViewportPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(volkGetLoadedDevice(), m_ViewportPipeline, nullptr);
        m_ViewportPipeline = VK_NULL_HANDLE;
    }

    if (m_PipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(volkGetLoadedDevice(), m_PipelineLayout, nullptr);
        m_PipelineLayout = VK_NULL_HANDLE;
    }

    if (m_PipelineCache != VK_NULL_HANDLE)
    {
        vkDestroyPipelineCache(volkGetLoadedDevice(), m_PipelineCache, nullptr);
        m_PipelineCache = VK_NULL_HANDLE;
    }

    if (m_DescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(volkGetLoadedDevice(), m_DescriptorSetLayout, nullptr);
        m_DescriptorSetLayout = VK_NULL_HANDLE;
    }

    if (m_DescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(volkGetLoadedDevice(), m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }

    m_DescriptorSets.clear();
}

VkRenderPass const& PipelineManager::GetMainRenderPass() const
{
    return m_MainRenderPass;
}

VkRenderPass const& PipelineManager::GetViewportRenderPass() const
{
    return m_ViewportRenderPass;
}

VkPipeline const& PipelineManager::GetMainPipeline() const
{
    return m_MainPipeline;
}

VkPipeline const& PipelineManager::GetViewportPipeline() const
{
    return m_ViewportPipeline;
}

VkPipelineLayout const& PipelineManager::GetPipelineLayout() const
{
    return m_PipelineLayout;
}

VkDescriptorSetLayout const& PipelineManager::GetDescriptorSetLayout() const
{
    return m_DescriptorSetLayout;
}

VkDescriptorPool const& PipelineManager::GetDescriptorPool() const
{
    return m_DescriptorPool;
}

VkDescriptorSet PipelineManager::GetDescriptorSet(std::uint32_t const ObjectID) const
{
    if (!m_DescriptorSets.contains(ObjectID))
    {
        return VK_NULL_HANDLE;
    }

    return m_DescriptorSets.at(ObjectID);
}