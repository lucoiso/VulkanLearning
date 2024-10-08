// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Types.UniformBufferObject;

export import RenderCore.Types.Vertex;
import RenderCore.Runtime.Device;

namespace RenderCore
{
    export struct RENDERCOREMODULE_API alignas(16) SceneUniformData
    {
        alignas(16) glm::vec3 LightPosition {};
        alignas(16) glm::vec3 LightColor {};
        alignas(16) glm::vec3 LightDiffuse {};
        alignas(16) glm::vec3 LightAmbient {};
        alignas(16) glm::vec3 LightSpecular {};
    };

    export struct RENDERCOREMODULE_API alignas(16) ModelUniformData
    {
        alignas(4)  std::uint32_t MeshletCount{ 0U };
        alignas(16) glm::mat4 ProjectionView {};
        alignas(16) glm::mat4 Model {};
    };

    static VkDeviceSize GetSceneUniformSize()
    {
        VkDeviceSize UniformSize = sizeof(SceneUniformData);

        if (VkDeviceSize const UniformAlignment = GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
            UniformAlignment > 0U)
        {
            UniformSize = UniformSize + UniformAlignment - 1U & ~(UniformAlignment - 1U);
        }
    }

    static VkDeviceSize GetModelUniformSize()
    {
        VkDeviceSize UniformSize = sizeof(ModelUniformData);

        if (VkDeviceSize const UniformAlignment = GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
            UniformAlignment > 0U)
        {
            UniformSize = UniformSize + UniformAlignment - 1U & ~(UniformAlignment - 1U);
        }
    }
} // namespace RenderCore
