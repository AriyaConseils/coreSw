#pragma once
/***************************************************************************************************
 * This file is part of a project developed by Ariya Consulting and Eymeric O'Neill.
 *
 * Copyright (C) [year] Ariya Consulting
 * Author/Creator: Eymeric O'Neill
 * Contact: +33 6 52 83 83 31
 * Email: eymeric.oneill@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 ***************************************************************************************************/

#include <initializer_list>
#include <type_traits>
#include <iostream>

// Classe pour des flags avec gestion des opérateurs bitwise
template<typename Enum>
class SwFlags {
    static_assert(std::is_enum<Enum>::value, "SwFlags can only be used with enumeration types.");

public:
    using IntType = typename std::underlying_type<Enum>::type;

private:
    IntType flags;  // Stocke les flags comme un entier

public:
    // Constructeurs
    constexpr SwFlags() noexcept : flags(0) {}
    constexpr SwFlags(Enum flag) noexcept : flags(static_cast<IntType>(flag)) {}
    constexpr SwFlags(std::initializer_list<Enum> flagList) noexcept : flags(computeFlags(flagList)) {}
	constexpr SwFlags(IntType flags) noexcept : flags(flags) {}

    // Méthodes de gestion
    constexpr void clear() noexcept { flags = 0; }
    constexpr void setFlag(Enum flag, bool on = true) noexcept {
        if (on) {
            flags |= static_cast<IntType>(flag);
        } else {
            flags &= ~static_cast<IntType>(flag);
        }
    }

    constexpr bool testFlag(Enum flag) const noexcept {
        return (flags & static_cast<IntType>(flag)) != 0;
    }

    constexpr IntType toInt() const noexcept { return flags; }

    constexpr operator IntType() const noexcept { return flags; }

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

    // Méthodes statiques
    static constexpr SwFlags fromInt(IntType value) noexcept {
        SwFlags flags;
        flags.flags = value;
        return flags;
    }

    // Utilitaires pour les opérateurs globaux
    friend constexpr SwFlags operator|(Enum lhs, Enum rhs) noexcept {
        return SwFlags(lhs) | rhs;
    }

    friend constexpr SwFlags operator&(Enum lhs, Enum rhs) noexcept {
        return SwFlags(lhs) & rhs;
    }

    friend constexpr SwFlags operator^(Enum lhs, Enum rhs) noexcept {
        return SwFlags(lhs) ^ rhs;
    }

    // Affichage pour le débogage
    friend std::ostream& operator<<(std::ostream& os, const SwFlags& flags) {
        os << flags.toInt();
        return os;
    }

private:
    // Fonction auxiliaire pour calculer les flags à partir d'une liste d'initialisation
    static constexpr IntType computeFlags(std::initializer_list<Enum> flagList) noexcept {
        IntType result = 0;
        for (Enum flag : flagList) {
            result |= static_cast<IntType>(flag);
        }
        return result;
    }
};

// Définir un macro pour déclarer des flags basés sur une énumération
#define SW_DECLARE_FLAGS(Flags, Enum) \
using Flags = SwFlags<Enum>;
