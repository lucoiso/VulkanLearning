// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/vulkan-renderer

module;

#include <cassert>
#include <type_traits>
#include <rps/rps.h>

export module RenderGraphRPS.Utils;

namespace RenderGraphRPS
{
    export template <typename T>
        requires std::is_same_v<T, RpsResult> || std::is_same_v<T, RpsResult &>
    constexpr void CheckRPSResult(T &&InputOperation)
    {
        assert(InputOperation == RPS_OK);
    }

    export template <typename T>
        requires std::is_invocable_v<T> && (std::is_same_v<std::invoke_result_t<T>, RpsResult> || std::is_same_v<
                                                std::invoke_result_t<T>, RpsResult &>)
    constexpr void CheckRPSResult(T &&InputOperation)
    {
        CheckRPSResult(InputOperation());
    }
} // namespace RenderGraphRPS
