// Author: Lucas Vilas-Boas
// Year : 2024
// Repo : https://github.com/lucoiso/VulkanRenderer

module;

#include <type_traits>

export module RenderCore.Utils.EnumHelpers;

#pragma region Operators
export template<typename T>
    requires std::is_enum_v<T>
constexpr T operator|(T Lhs, T Rhs)
{
    return static_cast<T>(static_cast<std::underlying_type_t<T>>(Lhs) | static_cast<std::underlying_type_t<T>>(Rhs));
}

export template<typename T>
    requires std::is_enum_v<T>
constexpr void operator|=(T &Lhs, T Rhs)
{
    Lhs = static_cast<T>(static_cast<std::underlying_type_t<T>>(Lhs) | static_cast<std::underlying_type_t<T>>(Rhs));
}

export template<typename T>
    requires std::is_enum_v<T>
constexpr T operator&(T Lhs, T Rhs)
{
    return static_cast<T>(static_cast<std::underlying_type_t<T>>(Lhs) & static_cast<std::underlying_type_t<T>>(Rhs));
}

export template<typename T>
    requires std::is_enum_v<T>
constexpr void operator&=(T &Lhs, T Rhs)
{
    Lhs = static_cast<T>(static_cast<std::underlying_type_t<T>>(Lhs) & static_cast<std::underlying_type_t<T>>(Rhs));
}

export template<typename T>
    requires std::is_enum_v<T>
constexpr T operator~(T Lhs)
{
    return static_cast<T>(~static_cast<std::underlying_type_t<T>>(Lhs));
}
#pragma endregion Operators

#pragma region Helpers
export template<typename T>
    requires std::is_enum_v<T>
constexpr void AddFlags(T &Lhs, T const Rhs)
{
    Lhs = Lhs | Rhs;
}

export template<typename T>
    requires std::is_enum_v<T>
constexpr void RemoveFlags(T &Lhs, T const Rhs)
{
    Lhs = Lhs & ~Rhs;
}

export template<typename T>
    requires std::is_enum_v<T>
constexpr bool HasFlag(T const Lhs, T const Rhs)
{
    return (Lhs & Rhs) == Rhs;
}

export template<typename T>
    requires std::is_enum_v<T>
constexpr bool HasAnyFlag(T const Lhs, T const Rhs)
{
    return (Lhs & Rhs) != T(0);
}

export template<typename T>
    requires std::is_enum_v<T>
constexpr bool HasAnyFlag(T const Lhs)
{
    return Lhs != T(0);
}
#pragma endregion Helpers
