// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <ADLX.h>
#include <type_traits>
#include "RadeonManagerModule.hpp"

export module RadeonManager.Manager;

export namespace RadeonManager
{
    RADEONMANAGERMODULE_API [[nodiscard]] bool IsRunning();

    RADEONMANAGERMODULE_API [[nodiscard]] bool IsLoaded();

    RADEONMANAGERMODULE_API [[nodiscard]] bool Start();

    RADEONMANAGERMODULE_API void Stop();

    RADEONMANAGERMODULE_API [[nodiscard]] adlx::IADLXSystem *GetADLXSystemServices();

    template <typename T>
        requires std::is_same_v<T, ADLX_RESULT> || std::is_same_v<T, ADLX_RESULT &>
    constexpr bool CheckADLXResult(T &&InputOperation)
    {
        return ADLX_SUCCEEDED(InputOperation);
    }

    template <typename T>
        requires std::is_invocable_v<T> && (std::is_same_v<std::invoke_result_t<T>, ADLX_RESULT> || std::is_same_v<
                                                std::invoke_result_t<T>, ADLX_RESULT &>)
    constexpr bool CheckADLXResult(T &&InputOperation)
    {
        return CheckVulkanResult(InputOperation());
    }
} // namespace RadeonManager
