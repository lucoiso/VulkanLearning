// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanRender

#pragma once

#include <cstdint>
#include <type_traits>

template<typename T>
constexpr T operator|(T const Lhs, T const Rhs)
{
    return static_cast<T>(static_cast<std::underlying_type_t<T>>(Lhs) | static_cast<std::underlying_type_t<T>>(Rhs));
}

template<typename T>
constexpr T operator|=(T& Lhs, T const Rhs)
{
    return Lhs = static_cast<T>(static_cast<std::underlying_type_t<T>>(Lhs) | static_cast<std::underlying_type_t<T>>(Rhs));
}

template<typename T>
constexpr T operator&(T const Lhs, T const Rhs)
{
    return static_cast<T>(static_cast<std::underlying_type_t<T>>(Lhs) & static_cast<std::underlying_type_t<T>>(Rhs));
}

template<typename T>
constexpr T operator&=(T& Lhs, T const Rhs)
{
    return Lhs = static_cast<T>(static_cast<std::underlying_type_t<T>>(Lhs) & static_cast<std::underlying_type_t<T>>(Rhs));
}

template<typename T>
constexpr T operator~(T const Lhs)
{
    return static_cast<T>(~static_cast<std::underlying_type_t<T>>(Lhs));
}
