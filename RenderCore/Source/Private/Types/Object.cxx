// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.Types.Object;

import RenderCore.Renderer;
import RenderCore.Runtime.Memory;
import RenderCore.Runtime.Pipeline;
import RenderCore.Types.UniformBufferObject;

using namespace RenderCore;

Object::Object(std::uint32_t const ID, strzilla::string_view const Path)
    : Resource(ID, Path)
{
}

Object::Object(std::uint32_t const ID, strzilla::string_view const Path, strzilla::string_view const Name)
    : Resource(ID, Path, Name)
{
}

void Object::Destroy()
{
    Resource::Destroy();
    Renderer::RequestUnloadObjects({ GetID() });
}

void Object::SetupUniformDescriptor()
{
    m_UniformBufferInfo = GetAllocationBufferDescriptor(m_UniformOffset, sizeof(ModelUniformData));
    m_MappedData        = GetAllocationMappedData();
}

void Object::UpdateUniformBuffers() const
{
    if (!m_MappedData)
    {
        return;
    }

    if (m_IsRenderDirty)
    {
        constexpr auto ModelUBOSize = sizeof(ModelUniformData);

        ModelUniformData const UpdatedModelUBO {
                .Model = m_Transform.GetMatrix() * m_Mesh->GetTransform().GetMatrix(),
                .BaseColorFactor = m_Mesh->GetMaterialData().BaseColorFactor,
                .EmissiveFactor = m_Mesh->GetMaterialData().EmissiveFactor,
                .MetallicFactor = static_cast<double>(m_Mesh->GetMaterialData().MetallicFactor),
                .RoughnessFactor = static_cast<double>(m_Mesh->GetMaterialData().RoughnessFactor),
                .AlphaCutoff = static_cast<double>(m_Mesh->GetMaterialData().AlphaCutoff),
                .NormalScale = static_cast<double>(m_Mesh->GetMaterialData().NormalScale),
                .OcclusionStrength = static_cast<double>(m_Mesh->GetMaterialData().OcclusionStrength),
                .AlphaMode = static_cast<std::int32_t>(m_Mesh->GetMaterialData().AlphaMode),
                .DoubleSided = static_cast<std::int32_t>(m_Mesh->GetMaterialData().DoubleSided)
        };

        std::memcpy(static_cast<char *>(m_MappedData) + GetUniformOffset(), &UpdatedModelUBO, ModelUBOSize);
        m_IsRenderDirty = false;
    }
}

void Object::DrawObject(VkCommandBuffer const &CommandBuffer, VkPipelineLayout const &PipelineLayout, std::uint32_t const ObjectIndex) const
{
    if (!m_Mesh)
    {
        return;
    }

    auto const &[SceneData, ModelData, TextureData] = GetPipelineDescriptorData();

    std::array const BufferBindingInfos {
            VkDescriptorBufferBindingInfoEXT
            {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                    .address = SceneData.BufferDeviceAddress.deviceAddress,
                    .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
            },
            VkDescriptorBufferBindingInfoEXT {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                    .address = ModelData.BufferDeviceAddress.deviceAddress,
                    .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
            },
            VkDescriptorBufferBindingInfoEXT {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                    .address = TextureData.BufferDeviceAddress.deviceAddress,
                    .usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
            }
    };

    vkCmdBindDescriptorBuffersEXT(CommandBuffer, static_cast<std::uint32_t>(std::size(BufferBindingInfos)), std::data(BufferBindingInfos));

    constexpr std::array BufferIndices { 0U, 1U, 2U };

    constexpr auto NumTextures = static_cast<std::uint8_t>(TextureType::Count);

    std::array const BufferOffsets {
            SceneData.LayoutOffset,
            ObjectIndex * ModelData.LayoutSize + ModelData.LayoutOffset,
            ObjectIndex * NumTextures * TextureData.LayoutSize + TextureData.LayoutOffset
    };

    vkCmdSetDescriptorBufferOffsetsEXT(CommandBuffer,
                                       VK_PIPELINE_BIND_POINT_GRAPHICS,
                                       PipelineLayout,
                                       0U,
                                       static_cast<std::uint32_t>(std::size(BufferBindingInfos)),
                                       std::data(BufferIndices),
                                       std::data(BufferOffsets));

    m_Mesh->BindBuffers(CommandBuffer, std::empty(m_InstanceTransform) ? 1U : GetNumInstances());
}