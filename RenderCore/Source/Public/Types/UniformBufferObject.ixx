// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Types.UniformBufferObject;

export import RenderCore.Types.Vertex;

namespace RenderCore
{
    export struct RENDERCOREMODULE_API SceneUniformData
    {
        alignas(16) glm::vec3 LightPosition {};
        alignas(16) glm::vec3 LightColor {};
        alignas(16) glm::vec3 LightDiffuse {};
        alignas(16) glm::vec3 LightAmbient {};
        alignas(16) glm::vec3 LightSpecular {};
    };

    export struct RENDERCOREMODULE_API ModelUniformData
    {
        alignas(16) glm::mat4 ProjectionView {};
        alignas(16) glm::mat4 Model {};
        alignas(4)  std::uint32_t MeshletCount { 0U };
    };
} // namespace RenderCore
