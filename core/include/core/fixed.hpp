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

// Adapted from fpm by Make Lankamp, licensed under MIT: https://github.com/MikeLankamp/fpm
// Updated to support cpp20, std::format, improved constexpr support, and added additional features.

#pragma once

#include "core/core.hpp"

namespace raoe
{

    template <std::integral TUnderlying, uint8 TFractionBits = 8, std::integral TIntermediate = TUnderlying,
              bool TRounding = true>
    class fixed
    {
        static_assert(TFractionBits > 0, "Fraction bits must be greater than 0");
        static_assert(TFractionBits < sizeof(TUnderlying) * 8 - 1,
                      "Fraction bits must be less than the size of the underlying type, minus 1");
        static_assert(sizeof(TUnderlying) >= sizeof(TIntermediate),
                      "Underlying type must be greater than or equal to the intermediate type");
        static_assert(std::is_signed_v<TUnderlying> == std::is_signed_v<TIntermediate>,
                      "Underlying and intermediate types must be share signededness");

        static constexpr TIntermediate FRACTION_MULT = TIntermediate(1) << TFractionBits;
        TUnderlying m_value;

        struct raw_construct_tag
        {
        };
        constexpr inline fixed(TUnderlying value, raw_construct_tag)
            : m_value(value)
        {
        }

      public:
        inline fixed() noexcept = default;

        template <std::integral T>
        constexpr inline explicit fixed(T value) noexcept
            : m_value(static_cast<TUnderlying>(TIntermediate(value) * FRACTION_MULT))
        {
        }

        template <std::floating_point T>
        constexpr inline explicit fixed(T value) noexcept
        {
            if constexpr(TRounding)
            {
                m_value = static_cast<TUnderlying>((value >= 0.0) ? (val * FRACTION_MULT + T {0.5} : (val * FRACTION_MULT - T {0.5})))
            }
            else
            {
                m_value = static_cast<TUnderlying>(value * FRACTION_MULT);
            }
        }

        template <std::integral TOtherUnderlying, uint8 TOtherFractionBits = 8,
                  std::integral TOtherIntermediate = TUnderlying, bool TOtherRounding = true>
        constexpr inline fixed(
            fixed<TOtherUnderlying, TOtherFractionBits, TOtherIntermediate, TOtherRounding> other) noexcept
            : m_value(from_fixed<TOtherFractionBits>(other.m_value).m_value)
        {
        }

        template <uint8 TOtherFractionalBits, std::integral T>
            requires(TOtherFractionalBits > TFractionBits)
        static constexpr inline fixed from_fixed(T value) noexcept
        {
            if constexpr(TRounding)
            {
                return fixed(static_cast<TUnderlying>(value / T(1) << (TOtherFractionalBits - TFractionBits)) +
                                 (value / (T(1) << (TOtherFractionalBits - TFractionBits - 1)) % 2),
                             raw_construct_tag());
            }
            else
            {
                return fixed(static_cast<TUnderlying>(value / T(1) << (TOtherFractionalBits - TFractionBits)),
                             raw_construct_tag());
            }
        }

        template <uint8 TOtherFractionalBits, std::integral T>
            requires(TOtherFractionalBits <= TFractionBits)
        static constexpr inline fixed from_fixed(T value) noexcept
        {
            return fixed(static_cast<TUnderlying>(value * (T(1) << (TFractionBits - TOtherFractionalBits))),
                         raw_construct_tag());
        }

        static constexpr inline fixed make_from_raw(TUnderlying val) noexcept
        {
            return fixed(val, raw_construct_tag());
        }

        [[nodiscard]] inline TUnderlying raw() const { return m_value; }

        template <std::floating_point T>
        constexpr inline explicit operator T() const noexcept
        {
            return static_cast<T>(m_value) / FRACTION_MULT;
        }

        template <std::integral T>
        constexpr inline explicit operator T() const noexcept
        {
            return static_cast<T>(m_value / FRACTION_MULT);
        }

        [[nodiscard]] static constexpr fixed e() { return from_fixed<61>(6267931151224907085ll); }
        [[nodiscard]] static constexpr fixed pi() { return from_fixed<61>(7244019458077122842ll); }
        [[nodiscard]] static constexpr fixed half_pi() { return from_fixed<62>(7244019458077122842ll); }
        [[nodiscard]] static constexpr fixed two_pi() { return from_fixed<60>(7244019458077122842ll); }

        constexpr inline fixed operator-() const noexcept { return fixed(-m_value, raw_construct_tag()); }

        inline fixed& operator+=(fixed other) noexcept
        {
            m_value += other.m_value;
            return *this;
        }

        template <typename T>
            requires std::integral<T> || std::floating_point<T>
        inline fixed& operator+=(T other) noexcept
        {
            return (*this + fixed(other));
        }

        inline fixed& operator-=(fixed other) noexcept
        {
            m_value -= other.m_value;
            return *this;
        }

        template <typename T>
            requires std::integral<T> || std::floating_point<T>
        inline fixed& operator-=(T other) noexcept
        {
            return (*this - fixed(other));
        }

        inline fixed& operator*=(fixed other) noexcept
        {
            if(EnableRounding)
            {
                // Normal fixed-point multiplication is: x * y / 2**FractionBits.
                // To correctly round the last bit in the result, we need one more bit of information.
                // We do this by multiplying by two before dividing and adding the LSB to the real result.
                auto value = (static_cast<TIntermediate>(m_value) * other.m_value) / (FRACTION_MULT / 2);
                m_value = static_cast<TUnderlying>((value / 2) + (value % 2));
            }
            else
            {
                auto value = (static_cast<TIntermediate>(m_value) * other.m_value) / FRACTION_MULT;
                m_value = static_cast<TUnderlying>(value);
            }
            return *this;
        }

        template <typename T>
            requires std::integral<T> || std::floating_point<T>
        inline fixed& operator*=(T other) noexcept
        {
            return (*this * fixed(other));
        }

        inline fixed& operator/=(fixed other) noexcept
        {
            raoe::check_if(other.m_value != 0, "Fixed-point division by zero");
            if(EnableRounding)
            {
                // Normal fixed-point division is: x * 2**FractionBits / y.
                // To correctly round the last bit in the result, we need one more bit of information.
                // We do this by multiplying by two before dividing and adding the LSB to the real result.
                auto value = (static_cast<TIntermediate>(m_value) * FRACTION_MULT * 2) / other.m_value;
                m_value = static_cast<TUnderlying>((value / 2) + (value % 2));
            }
            else
            {
                auto value = (static_cast<TIntermediate>(m_value) * FRACTION_MULT) / other.m_value;
                m_value = static_cast<TUnderlying>(value);
            }
            return *this;
        }

        template <typename T>
            requires std::integral<T> || std::floating_point<T>
        inline fixed& operator/=(T other) noexcept
        {
            return (*this / fixed(other));
        }
    };

}