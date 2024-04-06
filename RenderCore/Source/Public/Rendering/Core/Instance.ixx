// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Runtime.Instance;

export namespace RenderCore
{
    [[nodiscard]] bool CreateVulkanInstance();
    void               DestroyVulkanInstance();
} // namespace RenderCore
