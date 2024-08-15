// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Integrations.Offscreen;

import RenderCore.Types.Allocation;
import RenderCore.Types.SurfaceProperties;
import RenderCore.Utils.Constants;

export namespace RenderCore
{
    void CreateOffscreenResources(SurfaceProperties const &);

    [[nodiscard]] std::array<ImageAllocation, g_ImageCount> const &GetOffscreenImages();

    void DestroyOffscreenImages();
} // namespace RenderCore
