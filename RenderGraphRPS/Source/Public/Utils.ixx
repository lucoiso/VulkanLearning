// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <rps/rps.h>
#include <type_traits>

export module RenderGraphRPS.Utils;

namespace RenderGraphRPS
{
    export template <typename T>
        requires std::is_same_v<T, RpsResult> || std::is_same_v<T, RpsResult &>
    constexpr bool CheckRPSResult(T &&InputOperation)
    {
        return InputOperation == RPS_OK;
    }

    export template <typename T>
        requires std::is_invocable_v<T> && (std::is_same_v<std::invoke_result_t<T>, RpsResult> || std::is_same_v<std::invoke_result_t<T>, RpsResult &>)
    constexpr bool CheckRPSResult(T &&InputOperation)
    {
        return CheckRPSResult(InputOperation());
    }
} // namespace RenderGraphRPS
