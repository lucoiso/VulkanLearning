// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <vector>

export module RenderCore.Integrations.Viewport;

import RenderCore.Types.Allocation;
import RenderCore.Types.SurfaceProperties;

export namespace RenderCore
{
#ifdef VULKAN_RENDERER_ENABLE_IMGUI
    void CreateViewportResources(SurfaceProperties const &);

    [[nodiscard]] std::vector<ImageAllocation> const &GetViewportImages();

    void DestroyViewportImages();
#endif
} // namespace RenderCore
