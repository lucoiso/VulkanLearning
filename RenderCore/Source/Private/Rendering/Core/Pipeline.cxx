// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <array>
#include <numeric>
#include <ranges>
#include <vector>
#include <vma/vk_mem_alloc.h>
#include <Volk/volk.h>

module RenderCore.Runtime.Pipeline;

import RenderCore.Utils.Helpers;
import RenderCore.Utils.Constants;
import RenderCore.Types.Vertex;
import RenderCore.Types.Object;
import RenderCore.Types.Mesh;
import RenderCore.Types.Texture;
import RenderCore.Types.Material;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Runtime.Memory;
import RenderCore.Runtime.ShaderCompiler;
import RenderCore.Runtime.SwapChain;
import RenderCore.Runtime.Scene;
import RenderCore.Runtime.Device;

using namespace RenderCore;

VkPipeline                                    g_MainPipeline { VK_NULL_HANDLE };
VkPipeline                                    g_VertexInputPipeline { VK_NULL_HANDLE };
VkPipeline                                    g_PreRasterizationPipeline { VK_NULL_HANDLE };
VkPipeline                                    g_FragmentOutputPipeline { VK_NULL_HANDLE };
VkPipeline                                    g_FragmentShaderPipeline { VK_NULL_HANDLE };
VkPipelineLayout                              g_PipelineLayout { VK_NULL_HANDLE };
VkPipelineCache                               g_PipelineCache { VK_NULL_HANDLE };
VkPipelineCache                               g_PipelineLibraryCache { VK_NULL_HANDLE };
PipelineDescriptorData                        g_DescriptorData {};
VkPhysicalDeviceDescriptorBufferPropertiesEXT g_DescriptorBufferProperties {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT
};

constexpr VkPipelineCreateFlags g_PipelineFlags = VK_PIPELINE_CREATE_LIBRARY_BIT_KHR | VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT |
                                                  VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

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

constexpr VkPipelineColorBlendAttachmentState g_RenderColorBlendAttachmentStates {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
};

constexpr VkPipelineCacheCreateInfo g_PipelineCacheCreateInfo { .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };

bool PipelineDescriptorData::IsValid() const
{
    return SceneData.IsValid() && ModelData.IsValid() && TextureData.IsValid();
}

void PipelineDescriptorData::DestroyResources(VmaAllocator const &Allocator)
{
    SceneData.DestroyResources(Allocator);
    ModelData.DestroyResources(Allocator);
    TextureData.DestroyResources(Allocator);
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
        VmaAllocator const &Allocator = GetAllocator();

        constexpr VkBufferUsageFlags BufferUsage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
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

        CreateBuffer(static_cast<std::uint32_t>(std::size(Objects)) * ModelData.LayoutSize,
                     BufferUsage,
                     "Model Descriptor Buffer",
                     ModelData.Buffer.Buffer,
                     ModelData.Buffer.Allocation);

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

        CreateBuffer(NumTextures * static_cast<std::uint32_t>(std::size(Objects)) * TextureData.LayoutSize,
                     BufferUsage,
                     "Texture Descriptor Buffer",
                     TextureData.Buffer.Buffer,
                     TextureData.Buffer.Allocation);

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
    for (std::shared_ptr<Object> const &ObjectIter : Objects)
    {
        {
            VkBufferDeviceAddressInfo const BufferDeviceAddressInfo {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                    .buffer = GetAllocationBuffer(ObjectIter->GetBufferIndex())
            };

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

            auto const &ImageDescriptor = (*MatchingTexture)->GetImageDescriptor();

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

void RenderCore::CreatePipeline()
{
    VkDevice const &LogicalDevice = GetLogicalDevice();
    CheckVulkanResult(vkCreatePipelineCache(LogicalDevice, &g_PipelineCacheCreateInfo, nullptr, &g_PipelineCache));

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

        std::vector<ShaderStageData> const &ShaderStages = GetStageData();

        std::vector<VkPipelineShaderStageCreateInfo> ShaderStagesInfo {};
        std::vector<VkShaderModuleCreateInfo>        ShaderModuleInfo {};

        for (auto const &[StageInfo, ShaderCode] : ShaderStages)
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

        VkGraphicsPipelineCreateInfo const FragmentShaderPipelineCreateInfo {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = &FragmentLibrary,
                .flags = g_PipelineFlags,
                .stageCount = static_cast<std::uint32_t>(std::size(ShaderStagesInfo)),
                .pStages = std::data(ShaderStagesInfo),
                .pMultisampleState = &g_MultisampleState,
                .pDepthStencilState = &g_DepthStencilState,
                .layout = g_PipelineLayout
        };

        CheckVulkanResult(vkCreateGraphicsPipelines(LogicalDevice,
                                                    g_PipelineCache,
                                                    1U,
                                                    &FragmentShaderPipelineCreateInfo,
                                                    nullptr,
                                                    &g_FragmentShaderPipeline));
    }

    // Main Pipeline
    {
        std::vector const Libraries { g_VertexInputPipeline, g_PreRasterizationPipeline, g_FragmentOutputPipeline, g_FragmentShaderPipeline };

        VkPipelineLibraryCreateInfoKHR PipelineLibraryCreateInfo {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR,
                .pNext = &RenderingCreateInfo,
                .libraryCount = static_cast<std::uint32_t>(std::size(Libraries)),
                .pLibraries = std::data(Libraries)
        };

        VkGraphicsPipelineCreateInfo const GraphicsPipelineCreateInfo {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = &PipelineLibraryCreateInfo,
                .flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
                .layout = g_PipelineLayout
        };

        CheckVulkanResult(vkCreateGraphicsPipelines(LogicalDevice, g_PipelineCache, 1U, &GraphicsPipelineCreateInfo, nullptr, &g_MainPipeline));
    }
}

void RenderCore::CreatePipelineLibraries()
{
    VkDevice const &LogicalDevice = GetLogicalDevice();
    CheckVulkanResult(vkCreatePipelineCache(LogicalDevice, &g_PipelineCacheCreateInfo, nullptr, &g_PipelineLibraryCache));

    // Vertex input library
    {
        VkGraphicsPipelineLibraryCreateInfoEXT VertexInputLibrary {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT,
                .flags = VK_GRAPHICS_PIPELINE_LIBRARY_VERTEX_INPUT_INTERFACE_BIT_EXT
        };

        auto const BindingDescription    = GetBindingDescriptors(0U);
        auto const AttributeDescriptions = GetAttributeDescriptions(0U,
                                                                    {
                                                                            VertexAttributes::Position,
                                                                            VertexAttributes::Normal,
                                                                            VertexAttributes::TextureCoordinate,
                                                                            VertexAttributes::Color,
                                                                            VertexAttributes::Tangent,
                                                                    });

        VkPipelineVertexInputStateCreateInfo const VertexInputState {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .vertexBindingDescriptionCount = 1U,
                .pVertexBindingDescriptions = &BindingDescription,
                .vertexAttributeDescriptionCount = static_cast<std::uint32_t>(std::size(AttributeDescriptions)),
                .pVertexAttributeDescriptions = std::data(AttributeDescriptions)
        };

        constexpr VkPipelineInputAssemblyStateCreateInfo InputAssemblyState {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                .primitiveRestartEnable = VK_FALSE
        };

        VkGraphicsPipelineCreateInfo const VertexInputCreateInfo {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = &VertexInputLibrary,
                .flags = g_PipelineFlags,
                .pVertexInputState = &VertexInputState,
                .pInputAssemblyState = &InputAssemblyState
        };

        CheckVulkanResult(vkCreateGraphicsPipelines(LogicalDevice,
                                                    g_PipelineLibraryCache,
                                                    1U,
                                                    &VertexInputCreateInfo,
                                                    nullptr,
                                                    &g_VertexInputPipeline));
    }

    // Pre rasterization library
    {
        VkGraphicsPipelineLibraryCreateInfoEXT PreRasterizationLibrary {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT,
                .flags = VK_GRAPHICS_PIPELINE_LIBRARY_PRE_RASTERIZATION_SHADERS_BIT_EXT,
        };

        std::vector<ShaderStageData> const &ShaderStages = GetStageData();

        std::vector<VkPipelineShaderStageCreateInfo> ShaderStagesInfo {};
        std::vector<VkShaderModuleCreateInfo>        ShaderModuleInfo {};

        for (auto const &[StageInfo, ShaderCode] : ShaderStages)
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

        VkExtent2D const &SwapChainExtent = GetSwapChainExtent();

        VkViewport const Viewport {
                .x = 0.F,
                .y = 0.F,
                .width = static_cast<float>(SwapChainExtent.width),
                .height = static_cast<float>(SwapChainExtent.height),
                .minDepth = 0.F,
                .maxDepth = 1.F
        };

        VkRect2D const Scissor { .offset = { 0, 0 }, .extent = SwapChainExtent };

        VkPipelineViewportStateCreateInfo const ViewportState {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .viewportCount = 1U,
                .pViewports = &Viewport,
                .scissorCount = 1U,
                .pScissors = &Scissor
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

        constexpr VkPipelineDynamicStateCreateInfo DynamicState {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                .dynamicStateCount = static_cast<uint32_t>(std::size(g_DynamicStates)),
                .pDynamicStates = std::data(g_DynamicStates)
        };

        VkGraphicsPipelineCreateInfo const PreRasterizationInfo {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = &PreRasterizationLibrary,
                .flags = g_PipelineFlags,
                .stageCount = static_cast<std::uint32_t>(std::size(ShaderStagesInfo)),
                .pStages = std::data(ShaderStagesInfo),
                .pViewportState = &ViewportState,
                .pRasterizationState = &RasterizationState,
                .pDynamicState = &DynamicState,
                .layout = g_PipelineLayout
        };

        CheckVulkanResult(vkCreateGraphicsPipelines(LogicalDevice,
                                                    g_PipelineLibraryCache,
                                                    1U,
                                                    &PreRasterizationInfo,
                                                    nullptr,
                                                    &g_PreRasterizationPipeline));
    }

    // Fragment output library
    {
        VkFormat const SwapChainImageFormat = GetSwapChainImageFormat();
        VkFormat const DepthFormat          = GetDepthImage().Format;

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
                .pAttachments = &g_RenderColorBlendAttachmentStates,
                .blendConstants = { 0.F, 0.F, 0.F, 0.F }
        };

        VkGraphicsPipelineCreateInfo const FragmentOutputCreateInfo {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = &FragmentOutputLibrary,
                .flags = g_PipelineFlags,
                .pMultisampleState = &g_MultisampleState,
                .pColorBlendState = &ColorBlendState,
                .layout = g_PipelineLayout
        };

        CheckVulkanResult(vkCreateGraphicsPipelines(LogicalDevice,
                                                    g_PipelineLibraryCache,
                                                    1U,
                                                    &FragmentOutputCreateInfo,
                                                    nullptr,
                                                    &g_FragmentOutputPipeline));
    }
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
    CheckVulkanResult(vkCreatePipelineLayout(LogicalDevice, &PipelineLayoutCreateInfo, nullptr, &g_PipelineLayout));
    g_DescriptorData.SetDescriptorLayoutSize();
}

void RenderCore::ReleasePipelineResources()
{
    VkDevice const &LogicalDevice = GetLogicalDevice();

    if (g_MainPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(LogicalDevice, g_MainPipeline, nullptr);
        g_MainPipeline = VK_NULL_HANDLE;
    }

    if (g_VertexInputPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(LogicalDevice, g_VertexInputPipeline, nullptr);
        g_VertexInputPipeline = VK_NULL_HANDLE;
    }

    if (g_PreRasterizationPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(LogicalDevice, g_PreRasterizationPipeline, nullptr);
        g_PreRasterizationPipeline = VK_NULL_HANDLE;
    }

    if (g_FragmentOutputPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(LogicalDevice, g_FragmentOutputPipeline, nullptr);
        g_FragmentOutputPipeline = VK_NULL_HANDLE;
    }

    if (g_FragmentShaderPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(LogicalDevice, g_FragmentShaderPipeline, nullptr);
        g_FragmentShaderPipeline = VK_NULL_HANDLE;
    }

    if (g_PipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(LogicalDevice, g_PipelineLayout, nullptr);
        g_PipelineLayout = VK_NULL_HANDLE;
    }

    if (g_PipelineCache != VK_NULL_HANDLE)
    {
        vkDestroyPipelineCache(LogicalDevice, g_PipelineCache, nullptr);
        g_PipelineCache = VK_NULL_HANDLE;
    }

    if (g_PipelineLibraryCache != VK_NULL_HANDLE)
    {
        vkDestroyPipelineCache(LogicalDevice, g_PipelineLibraryCache, nullptr);
        g_PipelineLibraryCache = VK_NULL_HANDLE;
    }

    VmaAllocator const &Allocator = GetAllocator();
    g_DescriptorData.DestroyResources(Allocator);
}

VkPipeline const &RenderCore::GetMainPipeline()
{
    return g_MainPipeline;
}

VkPipelineLayout const &RenderCore::GetPipelineLayout()
{
    return g_PipelineLayout;
}

PipelineDescriptorData &RenderCore::GetPipelineDescriptorData()
{
    return g_DescriptorData;
}
