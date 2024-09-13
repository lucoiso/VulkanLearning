// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

export module RenderCore.Runtime.Synchronization;

import RenderCore.Utils.Constants;

namespace RenderCore
{
    RENDERCOREMODULE_API std::array<VkSemaphore, g_ImageCount> g_ImageAvailableSemaphores{};
    RENDERCOREMODULE_API std::array<VkSemaphore, g_ImageCount> g_RenderFinishedSemaphores{};
    RENDERCOREMODULE_API std::array<VkFence, g_ImageCount> g_Fences{};
    RENDERCOREMODULE_API std::array<bool, g_ImageCount> g_FenceInUse{};
} // namespace RenderCore

export namespace RenderCore
{
    void ResetSemaphores();
    void ResetFenceStatus();
    void WaitAndResetFence(std::uint32_t);
    void CreateSynchronizationObjects();
    void ReleaseSynchronizationObjects();

    RENDERCOREMODULE_API inline void SetFenceWaitStatus(std::uint32_t const Index, bool const Value)
    {
        g_FenceInUse.at(Index) = Value;
    }

    RENDERCOREMODULE_API [[nodiscard]] inline bool const &GetFenceWaitStatus(std::uint32_t const Index)
    {
        return g_FenceInUse.at(Index);
    }

    RENDERCOREMODULE_API [[nodiscard]] inline VkSemaphore const &GetImageAvailableSemaphore(std::uint32_t const Index)
    {
        return g_ImageAvailableSemaphores.at(Index);
    }

    RENDERCOREMODULE_API [[nodiscard]] inline VkSemaphore const &GetRenderFinishedSemaphore(std::uint32_t const Index)
    {
        return g_RenderFinishedSemaphores.at(Index);
    }

    RENDERCOREMODULE_API [[nodiscard]] inline VkFence const &GetFence(std::uint32_t const Index)
    {
        return g_Fences.at(Index);
    }
} // namespace RenderCore
