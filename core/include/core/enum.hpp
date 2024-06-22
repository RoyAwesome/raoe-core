/*
Copyright 2022-2024 Roy Awesome's Open Engine (RAOE)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#pragma once

#include <type_traits>

namespace raoe::core
{
    template <typename Enum>
    concept is_integral_enum = std::is_enum<Enum>::value && std::is_integral<std::underlying_type_t<Enum>>::value;
}

template <raoe::core::is_integral_enum E>
constexpr E operator~(E rhs) noexcept
{
    return static_cast<E>(~static_cast<std::underlying_type_t<E>>(rhs));
}

template <raoe::core::is_integral_enum E>
constexpr E operator|(E lhs, E rhs) noexcept
{
    return static_cast<E>(static_cast<std::underlying_type_t<E>>(lhs) | static_cast<std::underlying_type_t<E>>(rhs));
}

template <raoe::core::is_integral_enum E>
constexpr E operator&(E lhs, E rhs) noexcept
{
    return static_cast<E>(static_cast<std::underlying_type_t<E>>(lhs) & static_cast<std::underlying_type_t<E>>(rhs));
}

template <raoe::core::is_integral_enum E>
constexpr E operator^(E lhs, E rhs) noexcept
{
    return static_cast<E>(static_cast<std::underlying_type_t<E>>(lhs) ^ static_cast<std::underlying_type_t<E>>(rhs));
}

template <raoe::core::is_integral_enum E>
constexpr E& operator&=(E& lhs, E rhs) noexcept
{
    return lhs = (lhs & rhs);
}

template <raoe::core::is_integral_enum E>
constexpr E& operator|=(E& lhs, E rhs) noexcept
{
    return lhs = (lhs | rhs);
}

template <raoe::core::is_integral_enum E>
constexpr E& operator^=(E& lhs, E rhs) noexcept
{
    return lhs = (lhs ^ rhs);
}

template <raoe::core::is_integral_enum E>
constexpr bool operator==(E lhs, E rhs) noexcept
{
    return static_cast<std::underlying_type_t<E>>(lhs) == static_cast<std::underlying_type_t<E>>(rhs);
}

template <raoe::core::is_integral_enum E>
constexpr bool operator!=(E lhs, E rhs) noexcept
{
    return static_cast<std::underlying_type_t<E>>(lhs) != static_cast<std::underlying_type_t<E>>(rhs);
}

template <raoe::core::is_integral_enum E>
constexpr auto underlying(E e)
{
    return static_cast<std::underlying_type_t<E>>(e);
}

namespace raoe::core
{
    template <is_integral_enum Enum>
    constexpr bool has_all_flags(Enum flags, Enum contains)
    {
        return (flags & contains) == contains;
    }

    template <is_integral_enum Enum>
    constexpr bool has_any_flags(Enum flags, Enum contains)
    {
        return (flags & contains) != (Enum)0;
    }

    template <is_integral_enum Enum>
    constexpr bool add_flags(Enum& flags, Enum flags_to_add)
    {
        return flags |= flags_to_add;
    }

    template <is_integral_enum Enum>
    constexpr bool remove_flags(Enum& flags, Enum flags_to_remove)
    {
        return flags &= ~flags_to_remove;
    }
}
