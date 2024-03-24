// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <Volk/volk.h>
#include <array>
#include <boost/log/trivial.hpp>
#include <ranges>

module RenderCore.Management.PipelineManagement;

import RenderCore.Management.ShaderManagement;
import RenderCore.Management.DeviceManagement;
import RenderCore.Management.BufferManagement;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Utils.Constants;
import RenderCore.Utils.Helpers;
import RuntimeInfo.Manager;

using namespace RenderCore;

void PipelineManager::CreatePipeline(VkFormat const    SwapChainImageFormat,
                                     VkFormat const    DepthFormat,
                                     VkExtent2D const& ViewportExtent)
{
    auto const _{RuntimeInfo::Manager::Get().PushCallstackWithCounter()};
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating vulkan graphics pipelines";

    VkPipelineLayoutCreateInfo const PipelineLayoutCreateInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                                                              .setLayoutCount = 1U,
                                                              .pSetLayouts = &m_DescriptorSetLayout};

    CheckVulkanResult(vkCreatePipelineLayout(volkGetLoadedDevice(), &PipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));

    constexpr VkPipelineCacheCreateInfo PipelineCacheCreateInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};

    CheckVulkanResult(vkCreatePipelineCache(volkGetLoadedDevice(), &PipelineCacheCreateInfo, nullptr, &m_PipelineCache));

    auto const BindingDescription    = GetBindingDescriptors();
    auto const AttributeDescriptions = GetAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo const VertexInputState{.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                                                                .vertexBindingDescriptionCount = static_cast<std::uint32_t>(std::size(BindingDescription)),
                                                                .pVertexBindingDescriptions = std::data(BindingDescription),
                                                                .vertexAttributeDescriptionCount = static_cast<std::uint32_t>(std::size(AttributeDescriptions)),
                                                                .pVertexAttributeDescriptions = std::data(AttributeDescriptions)};

    std::vector<VkPipelineShaderStageCreateInfo> const ShaderStages = GetStageInfos();

    constexpr VkPipelineInputAssemblyStateCreateInfo InputAssemblyState{.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                                                                        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                                                                        .primitiveRestartEnable = VK_FALSE};

    VkViewport const Viewport{.x = 0.F,
                              .y = 0.F,
                              .width = static_cast<float>(ViewportExtent.width),
                              .height = static_cast<float>(ViewportExtent.height),
                              .minDepth = 0.F,
                              .maxDepth = 1.F};

    VkRect2D const Scissor{.offset = {0,
                                      0},
                           .extent = ViewportExtent};

    VkPipelineViewportStateCreateInfo const ViewportState{.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                                                          .viewportCount = 1U,
                                                          .pViewports = &Viewport,
                                                          .scissorCount = 1U,
                                                          .pScissors = &Scissor};

    constexpr VkPipelineRasterizationStateCreateInfo RasterizationState{.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                                                                        .depthClampEnable = VK_FALSE,
                                                                        .rasterizerDiscardEnable = VK_FALSE,
                                                                        .polygonMode = VK_POLYGON_MODE_FILL,
                                                                        .cullMode = VK_CULL_MODE_BACK_BIT,
                                                                        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                                                                        .depthBiasEnable = VK_FALSE,
                                                                        .depthBiasConstantFactor = 0.F,
                                                                        .depthBiasClamp = 0.F,
                                                                        .depthBiasSlopeFactor = 0.F,
                                                                        .lineWidth = 1.F};

    constexpr VkPipelineMultisampleStateCreateInfo MultisampleState{.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                                                                    .rasterizationSamples = g_MSAASamples,
                                                                    .sampleShadingEnable = VK_FALSE,
                                                                    .minSampleShading = 1.F,
                                                                    .pSampleMask = nullptr,
                                                                    .alphaToCoverageEnable = VK_FALSE,
                                                                    .alphaToOneEnable = VK_FALSE};

    constexpr VkPipelineDynamicStateCreateInfo DynamicState{.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                                                            .dynamicStateCount = static_cast<uint32_t>(std::size(g_DynamicStates)),
                                                            .pDynamicStates = std::data(g_DynamicStates)};

    constexpr VkPipelineDepthStencilStateCreateInfo DepthStencilState{.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                                                                      .depthTestEnable = VK_TRUE,
                                                                      .depthWriteEnable = VK_TRUE,
                                                                      .depthCompareOp = VK_COMPARE_OP_LESS,
                                                                      .depthBoundsTestEnable = VK_FALSE,
                                                                      .stencilTestEnable = VK_FALSE,
                                                                      .front = {},
                                                                      .back = {},
                                                                      .minDepthBounds = 0.F,
                                                                      .maxDepthBounds = 1.F};

    constexpr VkPipelineColorBlendAttachmentState RenderColorBlendAttachmentStates{.blendEnable = VK_TRUE,
                                                                                   .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                                                                                   .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                                                                                   .colorBlendOp = VK_BLEND_OP_ADD,
                                                                                   .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                                                                                   .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                                                                                   .alphaBlendOp = VK_BLEND_OP_ADD,
                                                                                   .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                                                   VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

    constexpr VkPipelineColorBlendAttachmentState ViewportColorBlendAttachmentStates{.blendEnable = VK_TRUE,
                                                                                     .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                                                                                     .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                                                                                     .colorBlendOp = VK_BLEND_OP_ADD,
                                                                                     .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                                                                                     .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                                                                                     .alphaBlendOp = VK_BLEND_OP_ADD,
                                                                                     .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                                                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

    std::vector ColorAttachments{SwapChainImageFormat};

    std::vector SamplesAttachments{g_MSAASamples};

    std::vector<VkPipelineColorBlendAttachmentState> ColorBlendStates{};
    ColorBlendStates.reserve(2U);

    #ifdef VULKAN_RENDERER_ENABLE_IMGUI
    ColorAttachments.push_back(SwapChainImageFormat);
    SamplesAttachments.push_back(g_MSAASamples);
    ColorBlendStates.push_back(ViewportColorBlendAttachmentStates);
    #endif

    ColorBlendStates.push_back(ViewportColorBlendAttachmentStates);
    ColorBlendStates.shrink_to_fit();

    VkAttachmentSampleCountInfoAMD const AttachmentCountInfo{.sType = VK_STRUCTURE_TYPE_ATTACHMENT_SAMPLE_COUNT_INFO_AMD,
                                                             .pNext = nullptr,
                                                             .colorAttachmentCount = static_cast<std::uint32_t>(std::size(SamplesAttachments)),
                                                             .pColorAttachmentSamples = std::data(SamplesAttachments),
                                                             .depthStencilAttachmentSamples = g_MSAASamples};

    VkPipelineRenderingCreateInfoKHR const RenderingCreateInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
                                                               .pNext = &AttachmentCountInfo,
                                                               .colorAttachmentCount = static_cast<std::uint32_t>(std::size(ColorAttachments)),
                                                               .pColorAttachmentFormats = std::data(ColorAttachments),
                                                               .depthAttachmentFormat = DepthFormat};

    VkPipelineColorBlendStateCreateInfo const ColorBlendState{.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                                                              .logicOpEnable = VK_FALSE,
                                                              .logicOp = VK_LOGIC_OP_COPY,
                                                              .attachmentCount = static_cast<std::uint32_t>(std::size(ColorBlendStates)),
                                                              .pAttachments = std::data(ColorBlendStates),
                                                              .blendConstants = {0.F,
                                                                                 0.F,
                                                                                 0.F,
                                                                                 0.F}};

    VkPipelineCreationFeedback PipelineCreationFeedback{
        .flags = VK_PIPELINE_CREATION_FEEDBACK_VALID_BIT | VK_PIPELINE_CREATION_FEEDBACK_APPLICATION_PIPELINE_CACHE_HIT_BIT |
        VK_PIPELINE_CREATION_FEEDBACK_BASE_PIPELINE_ACCELERATION_BIT,
        .duration = 0U};

    VkPipelineCreationFeedbackCreateInfo const PipelineCreationFeedbackCreateInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO,
                                                                                  .pNext = &RenderingCreateInfo,
                                                                                  .pPipelineCreationFeedback = &PipelineCreationFeedback,
                                                                                  .pipelineStageCreationFeedbackCount = 0U,
                                                                                  .pPipelineStageCreationFeedbacks = nullptr};

    VkGraphicsPipelineCreateInfo const GraphicsPipelineCreateInfo{.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                                                                  .pNext = &PipelineCreationFeedbackCreateInfo,
                                                                  .stageCount = static_cast<std::uint32_t>(std::size(ShaderStages)),
                                                                  .pStages = std::data(ShaderStages),
                                                                  .pVertexInputState = &VertexInputState,
                                                                  .pInputAssemblyState = &InputAssemblyState,
                                                                  .pViewportState = &ViewportState,
                                                                  .pRasterizationState = &RasterizationState,
                                                                  .pMultisampleState = &MultisampleState,
                                                                  .pDepthStencilState = &DepthStencilState,
                                                                  .pColorBlendState = &ColorBlendState,
                                                                  .pDynamicState = &DynamicState,
                                                                  .layout = m_PipelineLayout,
                                                                  .renderPass = VK_NULL_HANDLE,
                                                                  .subpass = 0U,
                                                                  .basePipelineHandle = VK_NULL_HANDLE,
                                                                  .basePipelineIndex = -1};

    CheckVulkanResult(vkCreateGraphicsPipelines(volkGetLoadedDevice(), m_PipelineCache, 1U, &GraphicsPipelineCreateInfo, nullptr, &m_Pipeline));
}

void PipelineManager::CreateDescriptorSetLayout()
{
    auto const _{RuntimeInfo::Manager::Get().PushCallstackWithCounter()};
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating vulkan decriptor set layout";

    constexpr std::array LayoutBindings{VkDescriptorSetLayoutBinding{.binding = 0U,
                                                                     .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                                     .descriptorCount = 1U,
                                                                     .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                                                                     .pImmutableSamplers = nullptr},
                                        VkDescriptorSetLayoutBinding{.binding = 1U,
                                                                     .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                                     .descriptorCount = 1U,
                                                                     .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                                                                     .pImmutableSamplers = nullptr}};

    VkDescriptorSetLayoutCreateInfo const DescriptorSetLayoutInfo{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                                  .bindingCount = static_cast<std::uint32_t>(std::size(LayoutBindings)),
                                                                  .pBindings = std::data(LayoutBindings)};

    CheckVulkanResult(vkCreateDescriptorSetLayout(volkGetLoadedDevice(), &DescriptorSetLayoutInfo, nullptr, &m_DescriptorSetLayout));
}

void PipelineManager::CreateDescriptorPool(std::uint32_t const NumAllocations)
{
    auto const _{RuntimeInfo::Manager::Get().PushCallstackWithCounter()};
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating vulkan descriptor pool";

    std::array const DescriptorPoolSizes{VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                              .descriptorCount = NumAllocations},
                                         VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                              .descriptorCount = NumAllocations}};

    VkDescriptorPoolCreateInfo const DescriptorPoolCreateInfo{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                                              .maxSets = 2U * NumAllocations,
                                                              .poolSizeCount = static_cast<std::uint32_t>(std::size(DescriptorPoolSizes)),
                                                              .pPoolSizes = std::data(DescriptorPoolSizes)};

    CheckVulkanResult(vkCreateDescriptorPool(volkGetLoadedDevice(), &DescriptorPoolCreateInfo, nullptr, &m_DescriptorPool));
}

void PipelineManager::CreateDescriptorSets(std::vector<MeshBufferData> const& AllocatedObjects,
                                           VkSampler const&                   Sampler)
{
    auto const _{RuntimeInfo::Manager::Get().PushCallstackWithCounter()};
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Creating vulkan descriptor sets";

    auto const NumAllocations{static_cast<std::uint32_t>(std::size(AllocatedObjects))};

    if (NumAllocations == 0U)
    {
        return;
    }

    m_DescriptorSets.reserve(NumAllocations);

    VkDescriptorSetAllocateInfo const DescriptorSetAllocateInfo{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                                                .descriptorPool = m_DescriptorPool,
                                                                .descriptorSetCount = 1U,
                                                                .pSetLayouts = &m_DescriptorSetLayout};

    for (auto const& [ID, UniformBuffer, UniformBufferData, Textures] : AllocatedObjects)
    {
        VkDescriptorSet AllocatedSet{VK_NULL_HANDLE};
        CheckVulkanResult(vkAllocateDescriptorSets(volkGetLoadedDevice(), &DescriptorSetAllocateInfo, &AllocatedSet));

        m_DescriptorSets.emplace(ID, AllocatedSet);
    }

    for (auto Iterator = std::begin(AllocatedObjects); Iterator != std::end(AllocatedObjects); ++Iterator)
    {
        constexpr std::size_t UniformBufferSize = sizeof(UniformBufferObject);
        auto const            Index             = static_cast<std::uint32_t>(std::distance(std::begin(AllocatedObjects), Iterator));

        std::vector<VkWriteDescriptorSet>   WriteDescriptors{};
        std::vector<VkDescriptorBufferInfo> BufferInfos{};
        std::vector<VkDescriptorImageInfo>  ImageInfos{};

        BufferInfos.push_back(VkDescriptorBufferInfo{.buffer = AllocatedObjects.at(Index).UniformBuffer,
                                                     .offset = 0U,
                                                     .range = UniformBufferSize});

        for (auto const& View : AllocatedObjects.at(Index).Textures | std::views::values)
        {
            ImageInfos.push_back(VkDescriptorImageInfo{.sampler = Sampler,
                                                       .imageView = View,
                                                       .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR});
        }

        if (!std::empty(BufferInfos))
        {
            WriteDescriptors.push_back(VkWriteDescriptorSet{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                                            .dstSet = m_DescriptorSets.at(Iterator->ID),
                                                            .dstBinding = 0U,
                                                            .dstArrayElement = 0U,
                                                            .descriptorCount = static_cast<std::uint32_t>(std::size(BufferInfos)),
                                                            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                            .pBufferInfo = std::data(BufferInfos)});
        }

        if (!std::empty(ImageInfos))
        {
            WriteDescriptors.push_back(VkWriteDescriptorSet{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                                            .dstSet = m_DescriptorSets.at(Iterator->ID),
                                                            .dstBinding = 1U,
                                                            .dstArrayElement = 0U,
                                                            .descriptorCount = static_cast<std::uint32_t>(std::size(ImageInfos)),
                                                            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                            .pImageInfo = std::data(ImageInfos)});
        }

        if (!std::empty(WriteDescriptors))
        {
            vkUpdateDescriptorSets(volkGetLoadedDevice(), static_cast<std::uint32_t>(std::size(WriteDescriptors)), std::data(WriteDescriptors), 0U, nullptr);
        }
    }
}

void PipelineManager::ReleasePipelineResources()
{
    auto const _{RuntimeInfo::Manager::Get().PushCallstackWithCounter()};
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Releasing vulkan pipeline resources";

    ReleaseDynamicPipelineResources();
}

void PipelineManager::ReleaseDynamicPipelineResources()
{
    auto const _{RuntimeInfo::Manager::Get().PushCallstackWithCounter()};
    BOOST_LOG_TRIVIAL(info) << "[" << __func__ << "]: Releasing vulkan pipeline resources";

    if (m_Pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(volkGetLoadedDevice(), m_Pipeline, nullptr);
        m_Pipeline = VK_NULL_HANDLE;
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

VkPipeline const& PipelineManager::GetMainPipeline() const
{
    return m_Pipeline;
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
