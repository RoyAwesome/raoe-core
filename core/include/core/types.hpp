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

#include "check.hpp"

#include <algorithm>
#include <cstdint>
#include <span>
#include <type_traits>

#ifndef RAOE_DEBUG
#ifdef NDEBUG
#define RAOE_DEBUG 0
#else
#define RAOE_DEBUG 1
#endif
#endif

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

namespace raoe
{
    struct void_value
    {
    };

    template<typename T>
    struct unwrap_reference
    {
        using type = T;
    };

    template<typename T>
    struct unwrap_reference<std::reference_wrapper<T>>
    {
        using type = T;
    };

    template<typename T>
    using unwrap_reference_t = typename unwrap_reference<T>::type;

    template<typename T>
    std::span<const std::byte> as_bytes(const T& o)
    {
        auto as_span_t = std::span<const T, 1>(std::addressof(o), 1);
        return std::as_bytes(as_span_t);
    }

    template<typename T>
    concept character =
        std::same_as<T, char> || std::same_as<T, signed char> || std::same_as<T, unsigned char> ||
        std::same_as<T, wchar_t> || std::same_as<T, char8_t> || std::same_as<T, char16_t> || std::same_as<T, char32_t>;

    // Taken from cppreference (https://en.cppreference.com/w/cpp/numeric/byteswap) and should be removed when updating
    // to cpp23
    template<std::integral T>
    constexpr T byteswap(T value) noexcept
    {
#if __cpp_lib_byteswap == 202110L
        return std::byteswap(value);
#else
        static_assert(std::has_unique_object_representations_v<T>, "T may not have padding bits");
        auto value_rep = std::bit_cast<std::array<uint8, sizeof(T)>>(value);
        std::ranges::reverse(value_rep);
        return std::bit_cast<T>(value_rep);
#endif
    }
}

// Hash Combine
namespace raoe
{
    inline namespace _
    {
        template<typename T>
        constexpr T xorshift(const T& n, int i) noexcept
        {
            return n ^ (n << i);
        }

        [[nodiscard]] inline constexpr uint32 distribute(uint32 n) noexcept
        {
            uint32 p = 0x55555555ul; // pattern of alternating 0 and 1
            uint32 c = 3423571495ul; // random uneven integer constant;
            return c * xorshift(p * xorshift(n, 16), 16);
        }

        [[nodiscard]] inline constexpr uint64 distribute(uint64 n) noexcept
        {
            const uint64 p = 0x5555555555555555ull;   // pattern of alternating 0 and 1
            const uint64 c = 17316035218449499591ull; // random uneven integer constant;
            return c * xorshift(p * xorshift(n, 32), 32);
        }
    }

    template<typename T>
    [[nodiscard]] inline constexpr std::size_t hash_combine(std::size_t seed, const T& v) noexcept
    {
        return std::rotl(seed, std::numeric_limits<std::size_t>::digits / 3) ^ distribute(std::hash<T> {}(v));
    }
}

// notnull from the C++ gsl
namespace raoe
{
    template<typename T>
    concept is_comparable_to_nullptr = std::is_convertible_v<T, decltype(std::declval<T>() != nullptr)>;

    template<class T>
        requires std::is_pointer_v<T>
    class not_null
    {
      public:
        static_assert(is_comparable_to_nullptr<T>, "T cannot be compared to nullptr");

        template<typename U>
            requires std::is_convertible_v<U, T>
        not_null(U&& u)
            : m_ptr(std::forward<U>(u))
        {
            raoe_check(m_ptr != nullptr);
        }

        template<typename U>
            requires std::is_convertible_v<U, T>
        not_null(U u)
            : m_ptr(u)
        {
            raoe_check(m_ptr != nullptr);
        }

        template<typename U>
            requires std::is_convertible_v<U, T>
        not_null(const not_null<U>& other)
            : m_ptr(other.get())
        {
        }

        not_null(const not_null& other) = default;
        not_null& operator=(const not_null& other) = default;

        [[nodiscard]] constexpr T get() const { return m_ptr; }
        [[nodiscard]] constexpr operator T() const { return get(); }

        [[nodiscard]] constexpr decltype(auto) operator->() const { return get(); }
        [[nodiscard]] constexpr decltype(auto) operator*() const { return *get(); }

        template<typename U>
            requires std::is_convertible_v<T, U>
        [[nodiscard]] constexpr explicit operator U() const
        {
            return static_cast<U>(get());
        }

        [[nodiscard]] constexpr bool operator==(const not_null& other) const { return get() == other.get(); }
        [[nodiscard]] constexpr bool operator!=(const not_null& other) const { return get() != other.get(); }

        template<typename U>
            requires std::is_convertible_v<T, U>
        [[nodiscard]] constexpr bool operator==(const not_null<U>& other) const
        {
            return get() == other.get();
        }

        template<typename U>
            requires std::is_convertible_v<T, U>
        [[nodiscard]] constexpr bool operator!=(const not_null<U>& other) const
        {
            return get() != other.get();
        }

        template<class U>
        auto operator<=(const not_null<U>& other) const noexcept(noexcept(std::less_equal<> {}(get(), other.get())))
            -> decltype(std::less_equal<> {}(get(), other.get()))
        {
            return std::less_equal<> {}(get(), other.get());
        }

        template<class U>
        auto operator>=(const not_null<U>& other) const noexcept(noexcept(std::greater_equal<> {}(get(), other.get())))
            -> decltype(std::greater_equal<> {}(get(), other.get()))
        {
            return std::greater_equal<> {}(get(), other.get());
        }

        template<class U>
        auto operator<(const not_null<U>& other) const noexcept(noexcept(std::less<> {}(get(), other.get())))
            -> decltype(std::less<> {}(get(), other.get()))
        {
            return std::less<> {}(get(), other.get());
        }

        template<class U>
        auto operator>(const not_null<U>& other) const noexcept(noexcept(std::greater<> {}(get(), other.get())))
            -> decltype(std::greater<> {}(get(), other.get()))
        {
            return std::greater<> {}(get(), other.get());
        }

        not_null(std::nullptr_t) = delete;
        not_null& operator=(std::nullptr_t) = delete;

        not_null& operator++() = delete;
        not_null& operator--() = delete;
        not_null operator++(int) = delete;
        not_null operator--(int) = delete;
        not_null& operator+=(std::ptrdiff_t) = delete;
        not_null& operator-=(std::ptrdiff_t) = delete;
        void operator[](std::ptrdiff_t) const = delete;

      private:
        T m_ptr;
    };
}

namespace std
{
    template<typename T>
    struct hash<raoe::not_null<T>>
    {
        size_t operator()(const raoe::not_null<T>& value) const noexcept { return hash<T> {}(value.get()); }
    };
}