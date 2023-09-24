// Author: Lucas Vilas-Boas
// Year : 2023
// Repo : https://github.com/lucoiso/VulkanLearning

#pragma once

#include <cstdint>

#define DECLARE_BITWISE_OPERATORS(Type)\
static inline Type operator|(const Type Lhs, const Type Rhs)\
{\
    return static_cast<Type>(static_cast<std::uint8_t>(Lhs) | static_cast<std::uint8_t>(Rhs));\
}\
static inline Type operator|=(Type& Lhs, const Type Rhs)\
{\
    return Lhs = static_cast<Type>(static_cast<std::uint8_t>(Lhs) | static_cast<std::uint8_t>(Rhs));\
}\
\
static inline Type operator&(const Type Lhs, const Type Rhs)\
{\
    return static_cast<Type>(static_cast<std::uint8_t>(Lhs) & static_cast<std::uint8_t>(Rhs));\
}\
\
static inline Type operator&=(Type& Lhs, const Type Rhs)\
{\
    return Lhs = static_cast<Type>(static_cast<std::uint8_t>(Lhs) & static_cast<std::uint8_t>(Rhs));\
}\
\
static inline Type operator~(const Type Lhs)\
{\
    return static_cast<Type>(~static_cast<std::uint8_t>(Lhs));\
}

namespace RenderCore
{
    enum class VulkanRenderCoreStateFlags : std::uint8_t
    {
        NONE                = 0,
        INITIALIZED         = 1 << 0,
        SCENE_LOADED        = 1 << 1,
        RENDERING           = 1 << 2,
        PENDING_REFRESH     = 1 << 3,
        SHUTDOWN            = 1 << 4,
        INVALID_RESOURCES   = 1 << 5,
        INVALID_PROPERTIES  = 1 << 6,
    };
    DECLARE_BITWISE_OPERATORS(VulkanRenderCoreStateFlags);
}

#undef DECLARE_BITWISE_OPERATORS