// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include "RadeonManagerModule.hpp"
#include <ADLX.h>
#include <type_traits>

export module RadeonManager.Manager;

export namespace RadeonManager
{
    [[nodiscard]] bool IsRunning();

    [[nodiscard]] bool IsLoaded();

    [[nodiscard]] bool Start();

    void Stop();

    [[nodiscard]] adlx::IADLXSystem* GetADLXSystemServices();

    template<typename T>
        requires std::is_same_v<T, ADLX_RESULT> || std::is_same_v<T, ADLX_RESULT&>
    constexpr bool CheckADLXResult(T&& InputOperation)
    {
        return ADLX_SUCCEEDED(InputOperation);
    }

    template<typename T>
        requires std::is_invocable_v<T> && (std::is_same_v<std::invoke_result_t<T>, ADLX_RESULT> || std::is_same_v<std::invoke_result_t<T>, ADLX_RESULT&>)
    constexpr bool CheckADLXResult(T&& InputOperation)
    {
        return CheckVulkanResult(InputOperation());
    }
}// namespace RadeonManager
