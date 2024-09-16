// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Runtime.Instance;

namespace RenderCore
{
    RENDERCOREMODULE_API VkInstance                g_Instance{VK_NULL_HANDLE};
    std::function<std::vector<strzilla::string>()> g_OnGetAdditionalInstanceExtensions{};
} // namespace RenderCore

export namespace RenderCore
{
    [[nodiscard]] bool CreateVulkanInstance();
    void               DestroyVulkanInstance();

    [[nodiscard]] RENDERCOREMODULE_API inline VkInstance const &GetInstance()
    {
        return g_Instance;
    }

    RENDERCOREMODULE_API inline void SetOnGetAdditionalInstanceExtensions(std::function<std::vector<strzilla::string>()> &&Callback)
    {
        g_OnGetAdditionalInstanceExtensions = std::move(Callback);
    }
} // namespace RenderCore
