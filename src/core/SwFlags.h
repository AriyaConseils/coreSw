#pragma once

#include <initializer_list>
#include <type_traits>
#include <iostream>

// SwFlag pour encapsuler un entier représentant une valeur de flag
class SwFlag {
    int value;
public:
    constexpr inline SwFlag(int v) noexcept : value(v) {}
    constexpr inline operator int() const noexcept { return value; }
};

// Classe pour des flags avec gestion des opérateurs bitwise
template<typename Enum>
class SwFlags {
    static_assert(std::is_enum<Enum>::value, "SwFlags can only be used with enumeration types.");
    using IntType = typename std::underlying_type<Enum>::type;

    IntType flags;  // Stocke les flags comme un entier

public:
    constexpr SwFlags() noexcept : flags(0) {}
    constexpr SwFlags(Enum flag) noexcept : flags(static_cast<IntType>(flag)) {}
    constexpr SwFlags(SwFlag flag) noexcept : flags(flag) {}

    // Ajouter des flags à l'aide d'une liste d'initialisation
    constexpr SwFlags(std::initializer_list<Enum> flagList) noexcept {
        flags = 0;
        for (Enum flag : flagList) {
            flags |= static_cast<IntType>(flag);
        }
    }

    // Opérateurs bitwise
    constexpr SwFlags& operator|=(Enum flag) noexcept {
        flags |= static_cast<IntType>(flag);
        return *this;
    }

    constexpr SwFlags& operator&=(Enum flag) noexcept {
        flags &= static_cast<IntType>(flag);
        return *this;
    }

    constexpr SwFlags& operator^=(Enum flag) noexcept {
        flags ^= static_cast<IntType>(flag);
        return *this;
    }

    constexpr SwFlags operator|(Enum flag) const noexcept {
        return SwFlags(flags | static_cast<IntType>(flag));
    }

    constexpr SwFlags operator&(Enum flag) const noexcept {
        return SwFlags(flags & static_cast<IntType>(flag));
    }

    constexpr SwFlags operator^(Enum flag) const noexcept {
        return SwFlags(flags ^ static_cast<IntType>(flag));
    }

    constexpr SwFlags operator~() const noexcept {
        return SwFlags(~flags);
    }

    constexpr bool operator==(SwFlags other) const noexcept {
        return flags == other.flags;
    }

    constexpr bool operator!=(SwFlags other) const noexcept {
        return flags != other.flags;
    }

    // Vérifie si un flag spécifique est activé
    constexpr bool testFlag(Enum flag) const noexcept {
        return (flags & static_cast<IntType>(flag)) == static_cast<IntType>(flag);
    }

    // Conversion en entier
    constexpr explicit operator IntType() const noexcept {
        return flags;
    }

    // Opérateurs globaux pour l'enum et SwFlags (implémentés dans la classe)
    friend constexpr SwFlags operator|(Enum lhs, Enum rhs) noexcept {
        return SwFlags(lhs) | rhs;
    }

    friend constexpr SwFlags operator&(Enum lhs, Enum rhs) noexcept {
        return SwFlags(lhs) & rhs;
    }

    friend constexpr SwFlags operator^(Enum lhs, Enum rhs) noexcept {
        return SwFlags(lhs) ^ rhs;
    }

    friend constexpr SwFlags operator|(Enum lhs, SwFlags rhs) noexcept {
        return rhs | lhs;
    }

    friend constexpr SwFlags operator&(Enum lhs, SwFlags rhs) noexcept {
        return rhs & lhs;
    }

    friend constexpr SwFlags operator^(Enum lhs, SwFlags rhs) noexcept {
        return rhs ^ lhs;
    }
};

// Définir un macro pour déclarer des flags basés sur une énumération
#define SW_DECLARE_FLAGS(Flags, Enum) \
    typedef SwFlags<Enum> Flags;
