// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <GLFW/glfw3.h>
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

    void AllocateScene(std::string_view);

    void ReleaseScene(std::vector<std::uint32_t> const &);

    void ReleaseSceneResources();

    void DestroyObjects();

    [[nodiscard]] ImageAllocation const &GetDepthImage();

    [[nodiscard]] VkSampler const &GetSampler();

    [[nodiscard]] ImageAllocation const &GetEmptyImage();

    void *GetSceneUniformData();

    [[nodiscard]] VkDescriptorBufferInfo const &GetSceneUniformDescriptor();

    [[nodiscard]] std::vector<std::shared_ptr<Object>> const &GetAllocatedObjects();

    [[nodiscard]] std::uint32_t GetNumAllocations();

    void UpdateSceneUniformBuffers(Camera const &, Illumination const &);
} // namespace RenderCore
