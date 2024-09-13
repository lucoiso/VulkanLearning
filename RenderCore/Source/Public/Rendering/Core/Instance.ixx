// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Runtime.Instance;

namespace RenderCore
{
    RENDERCOREMODULE_API VkInstance g_Instance { VK_NULL_HANDLE };
}

export namespace RenderCore
{
    [[nodiscard]] bool CreateVulkanInstance();
    void               DestroyVulkanInstance();

    [[nodiscard]] RENDERCOREMODULE_API inline VkInstance const &GetInstance()
    {
        return g_Instance;
    }
} // namespace RenderCore
