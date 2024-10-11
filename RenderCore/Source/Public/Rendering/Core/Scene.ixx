// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Runtime.Scene;

import RenderCore.Utils.Constants;
import RenderCore.Types.Camera;
import RenderCore.Types.Illumination;
import RenderCore.Types.Allocation;
import RenderCore.Types.Object;
import RenderCore.Types.SurfaceProperties;

namespace RenderCore
{
    RENDERCOREMODULE_API Camera                                              g_Camera {};
    RENDERCOREMODULE_API Illumination                                        g_Illumination {};
    RENDERCOREMODULE_API std::pair<BufferAllocation, VkDescriptorBufferInfo> m_UniformBufferAllocation {};
    RENDERCOREMODULE_API VkSampler                            g_Sampler { VK_NULL_HANDLE };
    RENDERCOREMODULE_API ImageAllocation                      g_DepthImage {};
    RENDERCOREMODULE_API std::atomic<std::uint64_t>           g_ObjectAllocationIDCounter { 0U };
    RENDERCOREMODULE_API std::vector<std::shared_ptr<Object>> g_Objects {};
}

export namespace RenderCore
{
    void CreateSceneUniformBuffer();
    void CreateImageSampler();
    void CreateDepthResources(SurfaceProperties const &);
    void AllocateEmptyTexture(VkFormat);
    void LoadScene(strzilla::string_view);
    void UnloadObjects(std::vector<std::uint32_t> const &);
    void ReleaseSceneResources();
    void DestroyObjects();
    void TickObjects(float);
    void UpdateSceneUniformBuffer();
    void UpdateObjectsUniformBuffer();

    RENDERCOREMODULE_API [[nodiscard]] inline std::uint32_t FetchID()
    {
        return g_ObjectAllocationIDCounter.fetch_add(1U);
    }

    RENDERCOREMODULE_API [[nodiscard]] inline ImageAllocation const &GetDepthImage()
    {
        return g_DepthImage;
    }

    RENDERCOREMODULE_API [[nodiscard]] inline VkSampler const &GetSampler()
    {
        return g_Sampler;
    }

    RENDERCOREMODULE_API [[nodiscard]] inline std::vector<std::shared_ptr<Object>> &GetObjects()
    {
        return g_Objects;
    }

    RENDERCOREMODULE_API [[nodiscard]] inline std::size_t GetNumAllocations()
    {
        return std::size(g_Objects);
    }

    RENDERCOREMODULE_API [[nodiscard]] inline BufferAllocation const &GetSceneUniformBuffer()
    {
        return m_UniformBufferAllocation.first;
    }

    RENDERCOREMODULE_API [[nodiscard]] inline void *GetSceneUniformData()
    {
        return m_UniformBufferAllocation.first.MappedData;
    }

    RENDERCOREMODULE_API [[nodiscard]] inline VkDescriptorBufferInfo const &GetSceneUniformDescriptor()
    {
        return m_UniformBufferAllocation.second;
    }

    RENDERCOREMODULE_API [[nodiscard]] inline Camera &GetCamera()
    {
        return g_Camera;
    }

    RENDERCOREMODULE_API [[nodiscard]] inline Illumination &GetIllumination()
    {
        return g_Illumination;
    }
} // namespace RenderCore
