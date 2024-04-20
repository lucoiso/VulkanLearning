// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <Volk/volk.h>

export module RenderCore.Runtime.Instance;

export namespace RenderCore
{
    [[nodiscard]] bool CreateVulkanInstance();
    void               DestroyVulkanInstance();

    [[nodiscard]] VkInstance const &GetInstance();
} // namespace RenderCore
