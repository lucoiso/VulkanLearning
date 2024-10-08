// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

module RenderCore.Types.Object;

import RenderCore.Renderer;
import RenderCore.Runtime.Memory;
import RenderCore.Runtime.Pipeline;
import RenderCore.Runtime.Scene;
import RenderCore.Types.Material;
import RenderCore.Types.Texture;
import RenderCore.Types.UniformBufferObject;
import RenderCore.Utils.Constants;

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

    // TODO : move to mesh

    auto const UniformData = static_cast<char *>(m_MappedData) + GetUniformOffset();

    if (IsRenderDirty())
    {
        constexpr auto ModelUBOSize = sizeof(ModelUniformData);

        ModelUniformData const UpdatedModelUBO {.MeshletCount   = GetMesh()->GetNumMeshlets(),
                                                .ProjectionView = GetCamera().GetProjectionMatrix(),
                                                .Model          = m_Transform.GetMatrix() * m_Mesh->GetTransform().GetMatrix()};

        std::memcpy(UniformData, &UpdatedModelUBO, ModelUBOSize);
        SetRenderDirty(false);
    }

    m_Mesh->UpdateUniformBuffers(UniformData);
}

void Object::DrawObject(VkCommandBuffer const &CommandBuffer, VkPipelineLayout const &PipelineLayout, std::uint32_t const ObjectIndex) const
{
    // TODO : move to mesh

    if (!m_Mesh)
    {
        return;
    }

    auto const &[SceneData,
                 ModelData,
                 MaterialData,
                 MeshletData,
                 TextureData] = GetPipelineDescriptorData();

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
                    .address = MaterialData.BufferDeviceAddress.deviceAddress,
                    .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
            },
            VkDescriptorBufferBindingInfoEXT {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                    .address = MeshletData.BufferDeviceAddress.deviceAddress,
                    .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
            },
            VkDescriptorBufferBindingInfoEXT {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
                    .address = TextureData.BufferDeviceAddress.deviceAddress,
                    .usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
            }
    };

    vkCmdBindDescriptorBuffersEXT(CommandBuffer, static_cast<std::uint32_t>(std::size(BufferBindingInfos)), std::data(BufferBindingInfos));

    constexpr std::array BufferIndices { 0U, 1U, 2U, 3U, 4U };
    constexpr auto NumTextures = static_cast<std::uint8_t>(TextureType::Count);

    std::array const BufferOffsets {
            SceneData.LayoutOffset,
            ObjectIndex * ModelData.LayoutSize    + ModelData.LayoutOffset,
            ObjectIndex * MaterialData.LayoutSize + MaterialData.LayoutOffset,
            ObjectIndex * MeshletData.LayoutSize  + MeshletData.LayoutOffset,
            ObjectIndex * NumTextures * TextureData.LayoutSize + TextureData.LayoutOffset
    };

    vkCmdSetDescriptorBufferOffsetsEXT(CommandBuffer,
                                       VK_PIPELINE_BIND_POINT_GRAPHICS,
                                       PipelineLayout,
                                       0U,
                                       static_cast<std::uint32_t>(std::size(BufferBindingInfos)),
                                       std::data(BufferIndices),
                                       std::data(BufferOffsets));

    std::uint32_t const NumMeshlets = m_Mesh->GetNumMeshlets();
    std::uint32_t const NumTasks = std::trunc(NumMeshlets + g_MaxMeshTasks - 1U) / g_MaxMeshTasks;

    if (vkCmdDrawMeshTasksNV != nullptr)
    {
        vkCmdDrawMeshTasksNV(CommandBuffer, NumTasks, 0U);
    }
    else
    {
        vkCmdDrawMeshTasksEXT(CommandBuffer, NumTasks, 1U, 1U);
    }
}