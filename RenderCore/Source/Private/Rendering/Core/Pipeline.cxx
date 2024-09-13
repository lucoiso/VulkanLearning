// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.Runtime.Pipeline;

import RenderCore.Runtime.Device;
import RenderCore.Runtime.Memory;
import RenderCore.Runtime.ShaderCompiler;
import RenderCore.Runtime.SwapChain;
import RenderCore.Runtime.Scene;
import RenderCore.Types.Allocation;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Types.Texture;
import RenderCore.Types.Vertex;
import RenderCore.Utils.Constants;
import RenderCore.Utils.Helpers;

using namespace RenderCore;

VkPhysicalDeviceDescriptorBufferPropertiesEXT g_DescriptorBufferProperties {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT
};

constexpr VkPipelineMultisampleStateCreateInfo g_MultisampleState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = g_MSAASamples,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.F,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
};

constexpr VkPipelineDepthStencilStateCreateInfo g_DepthStencilState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = {},
        .back = {},
        .minDepthBounds = 0.F,
        .maxDepthBounds = 1.F
};

constexpr VkPipelineCacheCreateInfo g_PipelineCacheCreateInfo { .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };

void PipelineData::DestroyResources(VkDevice const &LogicalDevice, bool const IncludeStatic)
{
    if (MainPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(LogicalDevice, MainPipeline, nullptr);
        MainPipeline = VK_NULL_HANDLE;
    }

    if (FragmentShaderPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(LogicalDevice, FragmentShaderPipeline, nullptr);
        FragmentShaderPipeline = VK_NULL_HANDLE;
    }

    if (!IncludeStatic)
    {
        return;
    }

    if (VertexInputPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(LogicalDevice, VertexInputPipeline, nullptr);
        VertexInputPipeline = VK_NULL_HANDLE;
    }

    if (PreRasterizationPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(LogicalDevice, PreRasterizationPipeline, nullptr);
        PreRasterizationPipeline = VK_NULL_HANDLE;
    }

    if (FragmentOutputPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(LogicalDevice, FragmentOutputPipeline, nullptr);
        FragmentOutputPipeline = VK_NULL_HANDLE;
    }

    if (PipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(LogicalDevice, PipelineLayout, nullptr);
        PipelineLayout = VK_NULL_HANDLE;
    }

    if (PipelineCache != VK_NULL_HANDLE)
    {
        vkDestroyPipelineCache(LogicalDevice, PipelineCache, nullptr);
        PipelineCache = VK_NULL_HANDLE;
    }

    if (PipelineLibraryCache != VK_NULL_HANDLE)
    {
        vkDestroyPipelineCache(LogicalDevice, PipelineLibraryCache, nullptr);
        PipelineLibraryCache = VK_NULL_HANDLE;
    }
}

void PipelineData::CreateMainCache(VkDevice const &LogicalDevice)
{
    if (g_PipelineData.PipelineCache == VK_NULL_HANDLE)
    {
        CheckVulkanResult(vkCreatePipelineCache(LogicalDevice, &g_PipelineCacheCreateInfo, nullptr, &PipelineCache));
    }
}

void PipelineData::CreateLibraryCache(VkDevice const &LogicalDevice)
{
    if (g_PipelineData.PipelineLibraryCache == VK_NULL_HANDLE)
    {
        CheckVulkanResult(vkCreatePipelineCache(LogicalDevice, &g_PipelineCacheCreateInfo, nullptr, &PipelineLibraryCache));
    }
}

void PipelineDescriptorData::DestroyResources(VmaAllocator const &Allocator, bool const IncludeStatic)
{
    SceneData.DestroyResources(Allocator, IncludeStatic);
    ModelData.DestroyResources(Allocator, IncludeStatic);
    TextureData.DestroyResources(Allocator, IncludeStatic);
}

void PipelineDescriptorData::SetDescriptorLayoutSize()
{
    VkPhysicalDeviceProperties2 DeviceProperties { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &g_DescriptorBufferProperties };
    vkGetPhysicalDeviceProperties2(GetPhysicalDevice(), &DeviceProperties);

    SceneData.SetDescriptorLayoutSize(g_DescriptorBufferProperties.descriptorBufferOffsetAlignment);
    ModelData.SetDescriptorLayoutSize(g_DescriptorBufferProperties.descriptorBufferOffsetAlignment);
    TextureData.SetDescriptorLayoutSize(g_DescriptorBufferProperties.descriptorBufferOffsetAlignment);
}

void PipelineDescriptorData::SetupSceneBuffer(BufferAllocation const &SceneAllocation)
{
    VkDevice const &LogicalDevice = GetLogicalDevice();

    {
        VmaAllocator const &         Allocator   = GetAllocator();
        constexpr VkBufferUsageFlags BufferUsage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

        SceneData.Buffer.Size = SceneData.LayoutSize;
        CreateBuffer(SceneData.LayoutSize, BufferUsage, "Scene Descriptor Buffer", SceneData.Buffer.Buffer, SceneData.Buffer.Allocation);
        vmaMapMemory(Allocator, SceneData.Buffer.Allocation, &SceneData.Buffer.MappedData);

        VkBufferDeviceAddressInfo const BufferDeviceAddressInfo {
                .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                .buffer = SceneData.Buffer.Buffer
        };

        SceneData.BufferDeviceAddress.deviceAddress = vkGetBufferDeviceAddress(LogicalDevice, &BufferDeviceAddressInfo);
    }

    {
        VkBufferDeviceAddressInfo const BufferDeviceAddressInfo {
                .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                .buffer = SceneAllocation.Buffer
        };

        VkDeviceSize const SceneUniformAddress = vkGetBufferDeviceAddress(LogicalDevice, &BufferDeviceAddressInfo);

        VkDescriptorAddressInfoEXT const SceneDescriptorAddressInfo {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
                .address = SceneUniformAddress,
                .range = sizeof(SceneUniformData),
                .format = VK_FORMAT_UNDEFINED
        };

        VkDescriptorGetInfoEXT const SceneDescriptorInfo {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .data = VkDescriptorDataEXT { .pUniformBuffer = &SceneDescriptorAddressInfo }
        };

        vkGetDescriptorEXT(LogicalDevice,
                           &SceneDescriptorInfo,
                           g_DescriptorBufferProperties.uniformBufferDescriptorSize,
                           static_cast<unsigned char *>(SceneData.Buffer.MappedData) + SceneData.LayoutOffset);
    }
}

void PipelineDescriptorData::SetupModelsBuffer(std::vector<std::shared_ptr<Object>> const &Objects)
{
    if (std::empty(Objects))
    {
        return;
    }

    VkDevice const &    LogicalDevice = GetLogicalDevice();
    VmaAllocator const &Allocator     = GetAllocator();

    {
        constexpr VkBufferUsageFlags BufferUsage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

        ModelData.Buffer.Size = static_cast<std::uint32_t>(std::size(Objects)) * ModelData.LayoutSize;

        CreateBuffer(ModelData.Buffer.Size, BufferUsage, "Model Descriptor Buffer", ModelData.Buffer.Buffer, ModelData.Buffer.Allocation);

        vmaMapMemory(Allocator, ModelData.Buffer.Allocation, &ModelData.Buffer.MappedData);

        VkBufferDeviceAddressInfo const BufferDeviceAddressInfo {
                .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                .buffer = ModelData.Buffer.Buffer
        };

        ModelData.BufferDeviceAddress.deviceAddress = vkGetBufferDeviceAddress(LogicalDevice, &BufferDeviceAddressInfo);
    }

    constexpr std::uint8_t NumTextures = static_cast<std::uint8_t>(TextureType::Count);

    {
        constexpr VkBufferUsageFlags BufferUsage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT |
                                                   VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

        TextureData.Buffer.Size = NumTextures * static_cast<std::uint32_t>(std::size(Objects)) * TextureData.LayoutSize;
        CreateBuffer(TextureData.Buffer.Size, BufferUsage, "Texture Descriptor Buffer", TextureData.Buffer.Buffer, TextureData.Buffer.Allocation);

        vmaMapMemory(Allocator, TextureData.Buffer.Allocation, &TextureData.Buffer.MappedData);

        VkBufferDeviceAddressInfo const BufferDeviceAddressInfo {
                .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                .buffer = TextureData.Buffer.Buffer
        };

        TextureData.BufferDeviceAddress.deviceAddress = vkGetBufferDeviceAddress(LogicalDevice, &BufferDeviceAddressInfo);
    }

    auto const ModelBuffer   = static_cast<unsigned char *>(ModelData.Buffer.MappedData);
    auto const TextureBuffer = static_cast<unsigned char *>(TextureData.Buffer.MappedData);

    std::uint32_t ObjectCount = 0U;

    VkBufferDeviceAddressInfo const BufferDeviceAddressInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = GetAllocationBuffer()
    };

    for (std::shared_ptr<Object> const &ObjectIter : Objects)
    {
        {
            VkDeviceSize const ModelUniformAddress = vkGetBufferDeviceAddress(LogicalDevice, &BufferDeviceAddressInfo);

            VkDescriptorAddressInfoEXT ModelDescriptorAddressInfo {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
                    .address = ModelUniformAddress + ObjectIter->GetUniformOffset(),
                    .range = sizeof(ModelUniformData)
            };

            VkDescriptorGetInfoEXT const ModelDescriptorInfo {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .data = VkDescriptorDataEXT { .pUniformBuffer = &ModelDescriptorAddressInfo }
            };

            VkDeviceSize const BufferOffset = ObjectCount * ModelData.LayoutSize + ModelData.LayoutOffset;

            vkGetDescriptorEXT(LogicalDevice,
                               &ModelDescriptorInfo,
                               g_DescriptorBufferProperties.uniformBufferDescriptorSize,
                               ModelBuffer + BufferOffset);
        }

        auto const &  Textures     = ObjectIter->GetMesh()->GetTextures();
        std::uint32_t TextureCount = 0U;

        for (std::uint8_t TypeIter = 0U; TypeIter < NumTextures; ++TypeIter)
        {
            auto MatchingTexture = std::ranges::find_if(Textures,
                                                        [TypeIter](std::shared_ptr<Texture> const &Texture)
                                                        {
                                                            auto const Types = Texture->GetTypes();
                                                            return std::ranges::find_if(Types,
                                                                                        [TypeIter](TextureType const &TextureType)
                                                                                        {
                                                                                            return static_cast<std::uint8_t>(TextureType) == TypeIter;
                                                                                        }) != std::end(Types);
                                                        });

            auto const &ImageDescriptor = MatchingTexture != std::cend(Textures)
                                              ? (*MatchingTexture)->GetImageDescriptor()
                                              : GetAllocationImageDescriptor(0U);

            VkDescriptorGetInfoEXT const TextureDescriptorInfo {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                    .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .data = VkDescriptorDataEXT { .pCombinedImageSampler = &ImageDescriptor }
            };

            VkDeviceSize const BufferOffset = (TextureCount + ObjectCount * NumTextures) * TextureData.LayoutSize + TextureData.LayoutOffset;

            vkGetDescriptorEXT(LogicalDevice,
                               &TextureDescriptorInfo,
                               g_DescriptorBufferProperties.combinedImageSamplerDescriptorSize,
                               TextureBuffer + BufferOffset);

            ++TextureCount;
        }

        ++ObjectCount;
    }
}

void RenderCore::CreatePipelineDynamicResources()
{
    std::vector<VkPipelineShaderStageCreateInfo> ShaderStagesInfo {};
    std::vector<VkShaderModuleCreateInfo>        ShaderModuleInfo {};

    for (auto const &[StageInfo, ShaderCode] : GetStageData())
    {
        if (StageInfo.stage == VK_SHADER_STAGE_FRAGMENT_BIT)
        {
            auto const CodeSize                            = static_cast<std::uint32_t>(std::size(ShaderCode) * sizeof(std::uint32_t));
            ShaderStagesInfo.emplace_back(StageInfo).pNext = &ShaderModuleInfo.emplace_back(VkShaderModuleCreateInfo {
                                                                                                    .sType =
                                                                                                    VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                                                                                    .codeSize = CodeSize,
                                                                                                    .pCode = std::data(ShaderCode)
                                                                                            });
        }
    }

    CreateMainPipeline(g_PipelineData, ShaderStagesInfo, VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT, g_DepthStencilState, g_MultisampleState);
}

void RenderCore::CreatePipelineLibraries()
{
    std::vector<VkPipelineShaderStageCreateInfo> ShaderStagesInfo {};
    std::vector<VkShaderModuleCreateInfo>        ShaderModuleInfo {};

    for (auto const &[StageInfo, ShaderCode] : GetStageData())
    {
        if (StageInfo.stage == VK_SHADER_STAGE_VERTEX_BIT)
        {
            auto const CodeSize                            = static_cast<std::uint32_t>(std::size(ShaderCode) * sizeof(std::uint32_t));
            ShaderStagesInfo.emplace_back(StageInfo).pNext = &ShaderModuleInfo.emplace_back(VkShaderModuleCreateInfo {
                                                                                                    .sType =
                                                                                                    VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                                                                                    .codeSize = CodeSize,
                                                                                                    .pCode = std::data(ShaderCode)
                                                                                            });
        }
    }

    constexpr VkPipelineColorBlendAttachmentState ColorBlendAttachmentStates {
            .blendEnable = VK_TRUE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    constexpr VkPipelineRasterizationStateCreateInfo RasterizationState {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0.F,
            .depthBiasClamp = 0.F,
            .depthBiasSlopeFactor = 0.F,
            .lineWidth = 1.F
    };

    PipelineLibraryCreationArguments const Arguments {
            .RasterizationState = RasterizationState,
            .ColorBlendAttachment = ColorBlendAttachmentStates,
            .MultisampleState = g_MultisampleState,
            .VertexBinding = GetBindingDescriptors(0U),
            .VertexAttributes = GetAttributeDescriptions(0U,
                                                         {
                                                                 VertexAttributes::Position,
                                                                 VertexAttributes::Normal,
                                                                 VertexAttributes::TextureCoordinate,
                                                                 VertexAttributes::Color,
                                                                 VertexAttributes::Tangent,
                                                         }),
            .ShaderStages = ShaderStagesInfo
    };

    CreatePipelineLibraries(g_PipelineData, Arguments, VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT, true);
}

void CreateDescriptorSetLayout(VkDescriptorSetLayoutBinding const &Binding, std::uint32_t const Bindings, VkDescriptorSetLayout &DescriptorSetLayout)
{
    std::vector LayoutBindings(Bindings, Binding);
    for (std::uint32_t Index = 0U; Index < Bindings; ++Index)
    {
        LayoutBindings.at(Index).binding = Index;
    }

    VkDescriptorSetLayoutCreateInfo const DescriptorSetLayoutInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
            .bindingCount = static_cast<std::uint32_t>(std::size(LayoutBindings)),
            .pBindings = std::data(LayoutBindings)
    };

    VkDevice const &LogicalDevice = GetLogicalDevice();
    CheckVulkanResult(vkCreateDescriptorSetLayout(LogicalDevice, &DescriptorSetLayoutInfo, nullptr, &DescriptorSetLayout));
}

void RenderCore::SetupPipelineLayouts()
{
    constexpr std::array LayoutBindings {
            VkDescriptorSetLayoutBinding // Uniform Buffer
            {
                    .binding = 0U,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1U,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                    .pImmutableSamplers = nullptr
            },
            VkDescriptorSetLayoutBinding // Texture Sampler
            {
                    .binding = 0U,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1U,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr
            }
    };

    CreateDescriptorSetLayout(LayoutBindings.at(0U), 1U, g_DescriptorData.SceneData.SetLayout);
    CreateDescriptorSetLayout(LayoutBindings.at(0U), 1U, g_DescriptorData.ModelData.SetLayout);
    CreateDescriptorSetLayout(LayoutBindings.at(1U), 5U, g_DescriptorData.TextureData.SetLayout);

    std::array const DescriptorLayouts {
            g_DescriptorData.SceneData.SetLayout,
            g_DescriptorData.ModelData.SetLayout,
            g_DescriptorData.TextureData.SetLayout
    };

    VkPipelineLayoutCreateInfo const PipelineLayoutCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = static_cast<std::uint32_t>(std::size(DescriptorLayouts)),
            .pSetLayouts = std::data(DescriptorLayouts)
    };

    VkDevice const &LogicalDevice = GetLogicalDevice();
    CheckVulkanResult(vkCreatePipelineLayout(LogicalDevice, &PipelineLayoutCreateInfo, nullptr, &g_PipelineData.PipelineLayout));
    g_DescriptorData.SetDescriptorLayoutSize();
}

void RenderCore::ReleasePipelineResources(bool const IncludeStatic)
{
    if (g_PipelineData.IsValid())
    {
        VkDevice const &LogicalDevice = GetLogicalDevice();
        g_PipelineData.DestroyResources(LogicalDevice, IncludeStatic);
    }

    VmaAllocator const &Allocator = GetAllocator();
    g_DescriptorData.DestroyResources(Allocator, IncludeStatic);
}

void RenderCore::CreatePipelineLibraries(PipelineData &                          Data,
                                         PipelineLibraryCreationArguments const &Arguments,
                                         VkPipelineCreateFlags const             Flags,
                                         bool const                              EnableDepth)
{
    VkDevice const &LogicalDevice = GetLogicalDevice();
    Data.CreateLibraryCache(LogicalDevice);

    // Vertex input library
    {
        VkGraphicsPipelineLibraryCreateInfoEXT VertexInputLibrary {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT,
                .flags = VK_GRAPHICS_PIPELINE_LIBRARY_VERTEX_INPUT_INTERFACE_BIT_EXT
        };

        VkPipelineVertexInputStateCreateInfo const VertexInputState {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .vertexBindingDescriptionCount = 1U,
                .pVertexBindingDescriptions = &Arguments.VertexBinding,
                .vertexAttributeDescriptionCount = static_cast<std::uint32_t>(std::size(Arguments.VertexAttributes)),
                .pVertexAttributeDescriptions = std::data(Arguments.VertexAttributes)
        };

        constexpr VkPipelineInputAssemblyStateCreateInfo InputAssemblyState {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                .primitiveRestartEnable = VK_FALSE
        };

        VkGraphicsPipelineCreateInfo const VertexInputCreateInfo {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = &VertexInputLibrary,
                .flags = g_PipelineFlags | Flags,
                .pVertexInputState = &VertexInputState,
                .pInputAssemblyState = &InputAssemblyState
        };

        CheckVulkanResult(vkCreateGraphicsPipelines(LogicalDevice,
                                                    Data.PipelineLibraryCache,
                                                    1U,
                                                    &VertexInputCreateInfo,
                                                    nullptr,
                                                    &Data.VertexInputPipeline));
    }

    // Pre rasterization library
    {
        VkGraphicsPipelineLibraryCreateInfoEXT PreRasterizationLibrary {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT,
                .flags = VK_GRAPHICS_PIPELINE_LIBRARY_PRE_RASTERIZATION_SHADERS_BIT_EXT,
        };

        constexpr VkPipelineViewportStateCreateInfo ViewportState {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .viewportCount = 1U,
                .scissorCount = 1U,
        };

        constexpr VkPipelineDynamicStateCreateInfo DynamicState {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                .dynamicStateCount = static_cast<uint32_t>(std::size(g_DynamicStates)),
                .pDynamicStates = std::data(g_DynamicStates)
        };

        VkGraphicsPipelineCreateInfo const PreRasterizationInfo {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = &PreRasterizationLibrary,
                .flags = g_PipelineFlags | Flags,
                .stageCount = static_cast<std::uint32_t>(std::size(Arguments.ShaderStages)),
                .pStages = std::data(Arguments.ShaderStages),
                .pViewportState = &ViewportState,
                .pRasterizationState = &Arguments.RasterizationState,
                .pDynamicState = &DynamicState,
                .layout = Data.PipelineLayout
        };

        CheckVulkanResult(vkCreateGraphicsPipelines(LogicalDevice,
                                                    Data.PipelineLibraryCache,
                                                    1U,
                                                    &PreRasterizationInfo,
                                                    nullptr,
                                                    &Data.PreRasterizationPipeline));
    }

    // Fragment output library
    {
        VkFormat const SwapChainImageFormat = GetSwapChainImageFormat();
        VkFormat const DepthFormat          = EnableDepth ? GetDepthImage().Format : VK_FORMAT_UNDEFINED;

        VkPipelineRenderingCreateInfo const RenderingCreateInfo {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .pNext = nullptr,
                .colorAttachmentCount = 1U,
                .pColorAttachmentFormats = &SwapChainImageFormat,
                .depthAttachmentFormat = DepthFormat,
                .stencilAttachmentFormat = DepthFormat
        };

        VkGraphicsPipelineLibraryCreateInfoEXT FragmentOutputLibrary {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT,
                .pNext = &RenderingCreateInfo,
                .flags = VK_GRAPHICS_PIPELINE_LIBRARY_FRAGMENT_OUTPUT_INTERFACE_BIT_EXT
        };

        VkPipelineColorBlendStateCreateInfo const ColorBlendState {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .logicOpEnable = VK_FALSE,
                .logicOp = VK_LOGIC_OP_COPY,
                .attachmentCount = 1U,
                .pAttachments = &Arguments.ColorBlendAttachment,
                .blendConstants = { 0.F, 0.F, 0.F, 0.F }
        };

        VkGraphicsPipelineCreateInfo const FragmentOutputCreateInfo {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = &FragmentOutputLibrary,
                .flags = g_PipelineFlags | Flags,
                .pMultisampleState = &Arguments.MultisampleState,
                .pColorBlendState = &ColorBlendState,
                .layout = Data.PipelineLayout
        };

        CheckVulkanResult(vkCreateGraphicsPipelines(LogicalDevice,
                                                    Data.PipelineLibraryCache,
                                                    1U,
                                                    &FragmentOutputCreateInfo,
                                                    nullptr,
                                                    &Data.FragmentOutputPipeline));
    }
}

void RenderCore::CreateMainPipeline(PipelineData &                                      Data,
                                    std::vector<VkPipelineShaderStageCreateInfo> const &FragmentStages,
                                    VkPipelineCreateFlags const                         Flags,
                                    VkPipelineDepthStencilStateCreateInfo const &       DepthStencilState,
                                    VkPipelineMultisampleStateCreateInfo const &        MultisampleState)
{
    VkDevice const &LogicalDevice = GetLogicalDevice();
    Data.CreateMainCache(LogicalDevice);

    VkFormat const SwapChainImageFormat = GetSwapChainImageFormat();
    VkFormat const DepthFormat          = GetDepthImage().Format;

    VkPipelineRenderingCreateInfo const RenderingCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1U,
            .pColorAttachmentFormats = &SwapChainImageFormat,
            .depthAttachmentFormat = DepthFormat,
            .stencilAttachmentFormat = DepthFormat
    };

    // Fragment shader library
    {
        VkGraphicsPipelineLibraryCreateInfoEXT FragmentLibrary {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT,
                .pNext = &RenderingCreateInfo,
                .flags = VK_GRAPHICS_PIPELINE_LIBRARY_FRAGMENT_SHADER_BIT_EXT
        };

        VkGraphicsPipelineCreateInfo const FragmentShaderPipelineCreateInfo {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = &FragmentLibrary,
                .flags = g_PipelineFlags | Flags,
                .stageCount = static_cast<std::uint32_t>(std::size(FragmentStages)),
                .pStages = std::data(FragmentStages),
                .pMultisampleState = &MultisampleState,
                .pDepthStencilState = &DepthStencilState,
                .layout = Data.PipelineLayout
        };

        CheckVulkanResult(vkCreateGraphicsPipelines(LogicalDevice,
                                                    Data.PipelineCache,
                                                    1U,
                                                    &FragmentShaderPipelineCreateInfo,
                                                    nullptr,
                                                    &Data.FragmentShaderPipeline));
    }

    // Main Pipeline
    {
        std::vector const Libraries {
                Data.VertexInputPipeline,
                Data.PreRasterizationPipeline,
                Data.FragmentOutputPipeline,
                Data.FragmentShaderPipeline
        };

        VkPipelineLibraryCreateInfoKHR PipelineLibraryCreateInfo {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR,
                .pNext = &RenderingCreateInfo,
                .libraryCount = static_cast<std::uint32_t>(std::size(Libraries)),
                .pLibraries = std::data(Libraries)
        };

        VkGraphicsPipelineCreateInfo const GraphicsPipelineCreateInfo {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = &PipelineLibraryCreateInfo,
                .flags = Flags,
                .layout = Data.PipelineLayout
        };

        CheckVulkanResult(vkCreateGraphicsPipelines(LogicalDevice, Data.PipelineCache, 1U, &GraphicsPipelineCreateInfo, nullptr, &Data.MainPipeline));
    }
}
