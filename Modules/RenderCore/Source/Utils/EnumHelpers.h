// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include <cstdint>

#define DECLARE_BITWISE_OPERATORS(Type)\
constexpr static inline Type operator|(const Type Lhs, const Type Rhs)\
{\
    return static_cast<Type>(static_cast<std::uint8_t>(Lhs) | static_cast<std::uint8_t>(Rhs));\
}\
constexpr static inline Type operator|=(Type& Lhs, const Type Rhs)\
{\
    return Lhs = static_cast<Type>(static_cast<std::uint8_t>(Lhs) | static_cast<std::uint8_t>(Rhs));\
}\
\
constexpr static inline Type operator&(const Type Lhs, const Type Rhs)\
{\
    return static_cast<Type>(static_cast<std::uint8_t>(Lhs) & static_cast<std::uint8_t>(Rhs));\
}\
\
constexpr static inline Type operator&=(Type& Lhs, const Type Rhs)\
{\
    return Lhs = static_cast<Type>(static_cast<std::uint8_t>(Lhs) & static_cast<std::uint8_t>(Rhs));\
}\
\
constexpr static inline Type operator~(const Type Lhs)\
{\
    return static_cast<Type>(~static_cast<std::uint8_t>(Lhs));\
}