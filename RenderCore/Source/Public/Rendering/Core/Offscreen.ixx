// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <vector>

export module RenderCore.Integrations.Offscreen;

import RenderCore.Types.Allocation;
import RenderCore.Types.SurfaceProperties;

export namespace RenderCore
{
    void CreateOffscreenResources(SurfaceProperties const &);

    [[nodiscard]] std::vector<ImageAllocation> const &GetOffscreenImages();

    void DestroyOffscreenImages();
} // namespace RenderCore
