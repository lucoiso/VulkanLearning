// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Runtime.Offscreen;

export import RenderCore.Utils.Constants;
export import RenderCore.Types.Allocation;
export import RenderCore.Types.SurfaceProperties;

namespace RenderCore
{
    RENDERCOREMODULE_API std::array<ImageAllocation, g_ImageCount> g_OffscreenImages {};
}

export namespace RenderCore
{
    void CreateOffscreenResources(SurfaceProperties const &);
    void DestroyOffscreenImages();

    RENDERCOREMODULE_API [[nodiscard]] inline std::array<ImageAllocation, g_ImageCount> const &GetOffscreenImages()
    {
        return g_OffscreenImages;
    }
} // namespace RenderCore
