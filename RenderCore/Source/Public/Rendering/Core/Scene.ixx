// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <vma/vk_mem_alloc.h>

export module RenderCore.Runtime.Scene;

import RenderCore.Types.Object;
import RenderCore.Types.Allocation;
import RenderCore.Types.SurfaceProperties;
import RenderCore.Types.Camera;
import RenderCore.Types.Illumination;

export namespace RenderCore
{
    void CreateSceneUniformBuffers();

    void CreateImageSampler();

    void CreateDepthResources(SurfaceProperties const &);

    void AllocateEmptyTexture(VkFormat);

    void LoadObject(std::string_view);

    void UnloadObjects(std::vector<std::uint32_t> const &);

    void ReleaseSceneResources();

    void DestroyObjects();

    [[nodiscard]] ImageAllocation const &GetDepthImage();

    [[nodiscard]] VkSampler const &GetSampler();

    [[nodiscard]] ImageAllocation const &GetEmptyImage();

    void *GetSceneUniformData();

    [[nodiscard]] VkDescriptorBufferInfo const &GetSceneUniformDescriptor();

    [[nodiscard]] std::vector<std::shared_ptr<Object>> const &GetObjects();

    [[nodiscard]] std::uint32_t GetNumAllocations();

    void UpdateSceneUniformBuffers();

    [[nodiscard]] Camera &GetCamera();

    [[nodiscard]] Illumination &GetIllumination();
} // namespace RenderCore
