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
import RenderCore.Types.Mesh;
import RenderCore.Types.Vertex;
import RenderCore.Types.Material;
import RenderCore.Utils.Constants;
import RenderCore.Utils.Helpers;

using namespace RenderCore;

VkPhysicalDeviceDescriptorBufferPropertiesEXT g_DescriptorBufferProperties {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT
};

constexpr VkPipelineMultisampleStateCreateInfo g_MultisampleState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = g_MSAASamples,
        .sampleShadingEnable = false,
        .minSampleShading = 1.F,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = false,
        .alphaToOneEnable = false
};

constexpr VkPipelineDepthStencilStateCreateInfo g_DepthStencilState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = true,
        .depthWriteEnable = true,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .depthBoundsTestEnable = false,
        .stencilTestEnable = false,
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

    if (FragmentPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(LogicalDevice, FragmentPipeline, nullptr);
        FragmentPipeline = VK_NULL_HANDLE;
    }

    if (!IncludeStatic)
    {
        return;
    }

    for (VkPipeline &LibraryIt : PipelineLibraries)
    {
        if (LibraryIt != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(LogicalDevice, LibraryIt, nullptr);
            LibraryIt = VK_NULL_HANDLE;
        }
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
    SceneData   .DestroyResources(Allocator, IncludeStatic);
    ModelData   .DestroyResources(Allocator, IncludeStatic);
    MaterialData.DestroyResources(Allocator, IncludeStatic);
    MeshletsData.DestroyResources(Allocator, IncludeStatic);
    IndicesData .DestroyResources(Allocator, IncludeStatic);
    VerticesData.DestroyResources(Allocator, IncludeStatic);
    TextureData .DestroyResources(Allocator, IncludeStatic);
}

void PipelineDescriptorData::SetDescriptorLayoutSize()
{
    VkPhysicalDeviceProperties2 DeviceProperties { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &g_DescriptorBufferProperties };
    vkGetPhysicalDeviceProperties2(GetPhysicalDevice(), &DeviceProperties);

    SceneData   .SetDescriptorLayoutSize(g_DescriptorBufferProperties.descriptorBufferOffsetAlignment);
    ModelData   .SetDescriptorLayoutSize(g_DescriptorBufferProperties.descriptorBufferOffsetAlignment);
    MaterialData.SetDescriptorLayoutSize(g_DescriptorBufferProperties.descriptorBufferOffsetAlignment);
    MeshletsData.SetDescriptorLayoutSize(g_DescriptorBufferProperties.descriptorBufferOffsetAlignment);
    IndicesData .SetDescriptorLayoutSize(g_DescriptorBufferProperties.descriptorBufferOffsetAlignment);
    VerticesData.SetDescriptorLayoutSize(g_DescriptorBufferProperties.descriptorBufferOffsetAlignment);
    TextureData .SetDescriptorLayoutSize(g_DescriptorBufferProperties.descriptorBufferOffsetAlignment);
}

void PipelineDescriptorData::SetupSceneBuffer(BufferAllocation const &SceneAllocation)
{
    VkDevice const &LogicalDevice = GetLogicalDevice();

    {
        VmaAllocator const          &Allocator   = GetAllocator();
        constexpr VkBufferUsageFlags BufferUsage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

        SceneData.Buffer.Size = SceneData.LayoutSize;
        CreateBuffer(SceneData.LayoutSize, BufferUsage, "Scene Descriptor Buffer", SceneData.Buffer.Buffer, SceneData.Buffer.Allocation);
        vmaMapMemory(Allocator, SceneData.Buffer.Allocation, &SceneData.Buffer.MappedData);

        VkBufferDeviceAddressInfo const BufferDeviceAddressInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = SceneData.Buffer.Buffer};

        SceneData.BufferDeviceAddress.deviceAddress = vkGetBufferDeviceAddress(LogicalDevice, &BufferDeviceAddressInfo);
    }

    {
        VkBufferDeviceAddressInfo const BufferDeviceAddressInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = SceneAllocation.Buffer};

        VkDeviceSize const SceneUniformAddress = vkGetBufferDeviceAddress(LogicalDevice, &BufferDeviceAddressInfo);

        VkDescriptorAddressInfoEXT const SceneDescriptorAddressInfo{.sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
                                                                    .address = SceneUniformAddress,
                                                                    .range   = sizeof(SceneUniformData),
                                                                    .format  = VK_FORMAT_UNDEFINED};

        VkDescriptorGetInfoEXT const SceneDescriptorInfo{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                                                         .type  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                         .data  = VkDescriptorDataEXT{.pUniformBuffer = &SceneDescriptorAddressInfo}};

        vkGetDescriptorEXT(LogicalDevice,
                           &SceneDescriptorInfo,
                           g_DescriptorBufferProperties.uniformBufferDescriptorSize,
                           static_cast<unsigned char *>(SceneData.Buffer.MappedData) + SceneData.LayoutOffset);
    }
}
void PipelineDescriptorData::SetupModelsBufferSizes(std::vector<std::shared_ptr<Object>> const &Objects)
{
    if (std::empty(Objects))
    {
        return;
    }

    VkDevice const     &LogicalDevice = GetLogicalDevice();
    VmaAllocator const &Allocator     = GetAllocator();

    auto const NumObjects = static_cast<std::uint32_t>(std::size(Objects));

    auto const SetupBufferDescriptor = [&](DescriptorData             &InData,
                                           strzilla::string_view const Identifier,
                                           std::uint32_t const         SizeMultiplier = 1U,
                                           VkBufferUsageFlags const    UsageFlags     = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT |
                                                                                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        InData.Buffer.Size = SizeMultiplier * NumObjects * InData.LayoutSize;

        CreateBuffer(InData.Buffer.Size, UsageFlags, std::data(Identifier), InData.Buffer.Buffer, InData.Buffer.Allocation);
        vmaMapMemory(Allocator, InData.Buffer.Allocation, &InData.Buffer.MappedData);

        VkBufferDeviceAddressInfo const BufferDeviceAddressInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = InData.Buffer.Buffer};

        InData.BufferDeviceAddress.deviceAddress = vkGetBufferDeviceAddress(LogicalDevice, &BufferDeviceAddressInfo);
    };

    // Projection
    SetupBufferDescriptor(ModelData, "Model Descriptor Buffer");

    // Materials
    SetupBufferDescriptor(MaterialData, "Material Descriptor Buffer");

    // Meshlets
    SetupBufferDescriptor(MeshletsData, "Meshlets Descriptor Buffer");

    // Indices
    SetupBufferDescriptor(IndicesData, "Indices Descriptor Buffer");

    // Vertices
    SetupBufferDescriptor(VerticesData, "Vertices Descriptor Buffer");

    // Textures
    {
        constexpr auto NumTextures = static_cast<std::uint8_t>(TextureType::Count);

        constexpr VkBufferUsageFlags BufferUsage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT |
                                                   VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
                                                   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;


        SetupBufferDescriptor(TextureData, "Texture Descriptor Buffer", NumTextures, BufferUsage);
    }
}

static void MapDescriptorBuffer(DescriptorData const   &Data,
                                unsigned char          *Buffer,
                                VkDeviceSize const     AddressInfo,
                                std::uint32_t const    ObjectCount,
                                std::uint32_t const    Offset,
                                std::uint32_t const    Size,
                                VkDescriptorType const Type)
{
    VkDevice const &LogicalDevice = GetLogicalDevice();

    VkDescriptorAddressInfoEXT const ModelDescriptorAddressInfo {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
                                                                 .address = AddressInfo + Offset,
                                                                 .range = Size};

    VkDescriptorDataEXT DescriptorData {};
    VkDeviceSize BufferDescriptorSize = 0U;
    switch (Type)
    {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        {
            DescriptorData.pUniformBuffer = &ModelDescriptorAddressInfo;
            BufferDescriptorSize = g_DescriptorBufferProperties.uniformBufferDescriptorSize;
            break;
        }
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        {
            DescriptorData.pStorageBuffer = &ModelDescriptorAddressInfo;
            BufferDescriptorSize = g_DescriptorBufferProperties.storageBufferDescriptorSize;
            break;
        }
        default:
        {
            BOOST_LOG_TRIVIAL(error) << "[" << __func__ << "]: Using unregistered descriptor type: " << Type;
            break;
        }
    }

    VkDescriptorGetInfoEXT const ModelDescriptorInfo{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                                                     .type  = Type,
                                                     .data  = DescriptorData };

    VkDeviceSize const BufferOffset = ObjectCount * Data.LayoutSize + Data.LayoutOffset;
    vkGetDescriptorEXT(LogicalDevice, &ModelDescriptorInfo, BufferDescriptorSize, Buffer + BufferOffset);
}

void PipelineDescriptorData::MapModelTextureBuffer(unsigned char *Buffer, std::shared_ptr<Object> const &Object, std::uint32_t const ObjectCount) const
{
    VkDevice const &LogicalDevice = GetLogicalDevice();
    constexpr auto NumTextures    = static_cast<std::uint8_t>(TextureType::Count);
    auto const     &Textures      = Object->GetMesh()->GetTextures();
    std::uint32_t   TextureCount  = 0U;

    for (std::uint8_t TypeIter = 0U; TypeIter < NumTextures; ++TypeIter)
    {
        auto const FindTexture = [&TypeIter] (std::shared_ptr<Texture> const &Texture)
        {
            auto const Types = Texture->GetTypes();

            return std::find_if(std::execution::unseq,
                                std::cbegin(Types),
                                std::cend(Types),
                                [&TypeIter](TextureType const &TextureType)
                                {
                                    return static_cast<std::uint8_t>(TextureType) == TypeIter;
                                }) != std::end(Types);
        };

        auto const MatchingTexture = std::find_if(std::execution::unseq, std::cbegin(Textures), std::cend(Textures), FindTexture);
        auto const &ImageDescriptor = MatchingTexture != std::cend(Textures) ? (*MatchingTexture)->GetImageDescriptor() : GetAllocationImageDescriptor(0U);

        VkDescriptorGetInfoEXT const TextureDescriptorInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .data = VkDescriptorDataEXT { .pCombinedImageSampler = &ImageDescriptor }
        };

        VkDeviceSize const BufferOffset = (TextureCount + ObjectCount * NumTextures) * TextureData.LayoutSize + TextureData.LayoutOffset;

        vkGetDescriptorEXT(LogicalDevice,
                           &TextureDescriptorInfo,
                           g_DescriptorBufferProperties.combinedImageSamplerDescriptorSize,
                           Buffer + BufferOffset);

        ++TextureCount;
    }
}

void PipelineDescriptorData::SetupModelsBuffer(std::vector<std::shared_ptr<Object>> const &Objects)
{
    if (std::empty(Objects))
    {
        return;
    }

    SetupModelsBufferSizes(Objects);

    VkDevice const &LogicalDevice = GetLogicalDevice();

    auto const ModelBuffer    = static_cast<unsigned char *>(ModelData.Buffer.MappedData);
    auto const MaterialBuffer = static_cast<unsigned char *>(MaterialData.Buffer.MappedData);
    auto const MeshletsBuffer = static_cast<unsigned char *>(MeshletsData.Buffer.MappedData);
    auto const IndicesBuffer  = static_cast<unsigned char *>(IndicesData.Buffer.MappedData);
    auto const VerticesBuffer = static_cast<unsigned char *>(VerticesData.Buffer.MappedData);
    auto const TextureBuffer  = static_cast<unsigned char *>(TextureData.Buffer.MappedData);

    std::uint32_t ObjectCount = 0U;

    VkBufferDeviceAddressInfo const UniformDeviceAddressInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = GetUniformAllocation().Buffer
    };

    VkDeviceSize const UniformAllocationAddress = vkGetBufferDeviceAddress(LogicalDevice, &UniformDeviceAddressInfo);

    VkBufferDeviceAddressInfo const StorageDeviceAddressInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = GetStorageAllocation().Buffer
    };

    VkDeviceSize const StorageAllocationAddress = vkGetBufferDeviceAddress(LogicalDevice, &StorageDeviceAddressInfo);

    for (std::shared_ptr<Object> const &ObjectIter : Objects)
    {
        auto const& ObjectMesh = ObjectIter->GetMesh();

        if (!ObjectMesh)
        {
            ++ObjectCount;
            continue;
        }

        MapDescriptorBuffer(ModelData,
                            ModelBuffer,
                            UniformAllocationAddress,
                            ObjectCount,
                            ObjectMesh->GetModelOffset(),
                            sizeof(ModelUniformData),
                            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

        MapDescriptorBuffer(MaterialData,
                            MaterialBuffer,
                            UniformAllocationAddress,
                            ObjectCount,
                            ObjectMesh->GetMaterialOffset(),
                            sizeof(RenderCore::MaterialData),
                            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

        MapDescriptorBuffer(MeshletsData,
                            MeshletsBuffer,
                            StorageAllocationAddress,
                            ObjectCount,
                            ObjectMesh->GetMeshletsOffset(),
                            ObjectMesh->GetNumMeshlets() * sizeof(Meshlet),
                            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

        MapDescriptorBuffer(IndicesData,
                            IndicesBuffer,
                            StorageAllocationAddress,
                            ObjectCount,
                            ObjectMesh->GetIndicesOffset(),
                            ObjectMesh->GetNumIndices() * sizeof(glm::uint8),
                            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

        MapDescriptorBuffer(VerticesData,
                            VerticesBuffer,
                            StorageAllocationAddress,
                            ObjectCount,
                            ObjectMesh->GetVerticesOffset(),
                            ObjectMesh->GetNumVertices() * sizeof(Vertex),
                            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

        MapModelTextureBuffer(TextureBuffer, ObjectIter, ObjectCount);
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
        if (StageInfo.stage == VK_SHADER_STAGE_MESH_BIT_EXT)
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
            .blendEnable = true,
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
            .depthClampEnable = false,
            .rasterizerDiscardEnable = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable = false,
            .depthBiasConstantFactor = 0.F,
            .depthBiasClamp = 0.F,
            .depthBiasSlopeFactor = 0.F,
            .lineWidth = 1.F
    };

    PipelineLibraryCreationArguments const Arguments {
            .RasterizationState = RasterizationState,
            .ColorBlendAttachment = ColorBlendAttachmentStates,
            .MultisampleState = g_MultisampleState,
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
    // TODO : add compute stage

    constexpr std::array LayoutBindings {
            VkDescriptorSetLayoutBinding // Scene Buffer
            {
                    .binding = 0U,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1U,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr
            },
            VkDescriptorSetLayoutBinding // Model Buffer
            {
                    .binding = 0U,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1U,
                    .stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT,
                    .pImmutableSamplers = nullptr
            },
            VkDescriptorSetLayoutBinding // Material Buffer
            {
                    .binding = 0U,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1U,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr
            },
            VkDescriptorSetLayoutBinding // Meshlets Buffer
            {
                    .binding = 0U,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1U,
                    .stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT,
                    .pImmutableSamplers = nullptr
            },
            VkDescriptorSetLayoutBinding // Indices Buffer
            {
                    .binding = 0U,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1U,
                    .stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT,
                    .pImmutableSamplers = nullptr
            },
            VkDescriptorSetLayoutBinding // Vertices Buffer
            {
                    .binding = 0U,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1U,
                    .stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT,
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

    CreateDescriptorSetLayout(LayoutBindings.at(0U), 1U, g_DescriptorData.SceneData.SetLayout); // Light
    CreateDescriptorSetLayout(LayoutBindings.at(1U), 1U, g_DescriptorData.ModelData.SetLayout); // Projection
    CreateDescriptorSetLayout(LayoutBindings.at(2U), 1U, g_DescriptorData.MaterialData.SetLayout); // Material
    CreateDescriptorSetLayout(LayoutBindings.at(3U), 1U, g_DescriptorData.MeshletsData.SetLayout); // Meshlets
    CreateDescriptorSetLayout(LayoutBindings.at(4U), 1U, g_DescriptorData.IndicesData.SetLayout); // Indices
    CreateDescriptorSetLayout(LayoutBindings.at(5U), 1U, g_DescriptorData.VerticesData.SetLayout); // Vertices
    CreateDescriptorSetLayout(LayoutBindings.at(6U), static_cast<std::uint8_t>(TextureType::Count), g_DescriptorData.TextureData.SetLayout); // Textures

    std::array const DescriptorLayouts {
            g_DescriptorData.SceneData.SetLayout,
            g_DescriptorData.ModelData.SetLayout,
            g_DescriptorData.MaterialData.SetLayout,
            g_DescriptorData.MeshletsData.SetLayout,
            g_DescriptorData.IndicesData.SetLayout,
            g_DescriptorData.VerticesData.SetLayout,
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
    if (!std::empty(Arguments.VertexAttributes))
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
                .primitiveRestartEnable = false
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
                                                    &Data.PipelineLibraries.at(0U)));
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
                                                    &Data.PipelineLibraries.at(1U)));
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
                .logicOpEnable = false,
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
                                                    &Data.PipelineLibraries.at(2U)));
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
                                                    &Data.FragmentPipeline));
    }

    // Main Pipeline
    {
        std::vector<VkPipeline> PipelineData {};
        PipelineData.reserve(std::size(Data.PipelineLibraries) + 1U);

        for (VkPipeline const& LibraryIt : Data.PipelineLibraries)
        {
            if (LibraryIt != VK_NULL_HANDLE)
            {
                PipelineData.emplace_back(LibraryIt);
            }
        }

        PipelineData.emplace_back(Data.FragmentPipeline);

        VkPipelineLibraryCreateInfoKHR PipelineLibraryCreateInfo {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR,
                .pNext = &RenderingCreateInfo,
                .libraryCount = static_cast<std::uint32_t>(std::size(PipelineData)),
                .pLibraries = std::data(PipelineData)
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
