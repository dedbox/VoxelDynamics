#pragma once

template <typename Enum>
struct is_bitmask_enum : std::false_type
{
};

template <typename Enum>
    requires is_bitmask_enum<Enum>::value
constexpr Enum operator|(Enum lhs, Enum rhs)
{
    using underlying = std::underlying_type_t<Enum>;
    return static_cast<Enum>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
}

template <typename Enum>
    requires is_bitmask_enum<Enum>::value
constexpr Enum operator&(Enum lhs, Enum rhs)
{
    using underlying = std::underlying_type_t<Enum>;
    return static_cast<Enum>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
}

template <typename Enum>
    requires is_bitmask_enum<Enum>::value
constexpr Enum operator^(Enum lhs, Enum rhs)
{
    using underlying = std::underlying_type_t<Enum>;
    return static_cast<Enum>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
}

template <typename Enum>
    requires is_bitmask_enum<Enum>::value
constexpr Enum operator~(Enum rhs)
{
    using underlying = std::underlying_type_t<Enum>;
    return static_cast<Enum>(~static_cast<underlying>(rhs));
}

template <typename Enum>
    requires is_bitmask_enum<Enum>::value
constexpr Enum operator|=(Enum lhs, Enum rhs)
{
    using underlying = std::underlying_type_t<Enum>;
    lhs = static_cast<Enum>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
}

template <typename Enum>
    requires is_bitmask_enum<Enum>::value
constexpr Enum operator&=(Enum lhs, Enum rhs)
{
    using underlying = std::underlying_type_t<Enum>;
    lhs = static_cast<Enum>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
}

template <typename Enum>
    requires is_bitmask_enum<Enum>::value
constexpr Enum operator^=(Enum lhs, Enum rhs)
{
    using underlying = std::underlying_type_t<Enum>;
    lhs = static_cast<Enum>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
}

template <typename Enum>
    requires is_bitmask_enum<Enum>::value
constexpr bool has_bits(Enum lhs, Enum rhs)
{
    using underlying = std::underlying_type_t<Enum>;
    return (static_cast<underlying>(lhs) & static_cast<underlying>(rhs)) != 0;
}
