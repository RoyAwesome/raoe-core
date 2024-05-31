/*
Copyright 2022 Roy Awesome's Open Engine (RAOE)

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

#if RAOE_CORE_USE_LIBASSERT
#include "assert.hpp"

#define raoe_check_impl(...) ASSERT(__VA_ARGS__);

#define raoe_check_always_impl(...) VERIFY(__VA_ARGS__);

#else

#ifndef raoe_check

#define raoe_check_impl(...) assert(__VA_ARGS__);

#define raoe_check_always_impl(...) assert(__VA_ARGS__);

#endif

#endif

#ifdef NDEBUG
#define raoe_check(...) ((void)0)
#else

#include "debug.hpp"

#define raoe_check(...)                                                                                                \
    raoe::debug::debug_break_if(!(!!(__VA_ARGS__)));                                                                   \
    raoe_check_impl(__VA_ARGS__)

#endif

#define raoe_check_always(...)                                                                                         \
    raoe::debug::debug_break_if((!!(__VA_ARGS__)));                                                                    \
    raoe_check_always_impl(__VA_ARGS__)

#ifdef RAOE_CORE_USE_SPDLOG
#include "spdlog/spdlog.h"
#endif

#include <cstdlib>
#include <source_location>
#include <string_view>
#include <format>
#include <type_traits>
#include <concepts>

namespace raoe
{
    namespace _internal
    {
        struct panic_sv
        {
            template <class T>
                requires std::constructible_from<std::string_view, T>
            panic_sv(const T& s, std::source_location loc = std::source_location::current()) noexcept
                : m_reason(s)
                , m_location(loc)
            {
            }

            std::string_view m_reason;
            std::source_location m_location;
        };

        template <typename... Args>
        struct panic_fmt
        {
            template <typename T>
            consteval panic_fmt(const T& t, std::source_location loc = std::source_location::current()) noexcept
                : m_reason(t)
                , m_location(loc)
            {
            }

            std::format_string<Args...> m_reason;
            std::source_location m_location;
        };
    }

    // Panics the program, printing the reason and location to the console
    // and breaking into the debugger if possible.
    // This function does not return, and will exit the program with a non-zero exit code.
    [[noreturn]] inline void panic(_internal::panic_sv reason) noexcept
    {
#ifdef RAOE_CORE_USE_SPDLOG
        spdlog::critical("!!!Panic!!!!\n\nReason: \"{}\"\n\nWhere:\n\t{}:{}:{}", reason.m_reason,
                         reason.m_location.file_name(), reason.m_location.line(), reason.m_location.column());
#endif
        raoe::debug::debug_break();
        std::abort();
    }

    // Panics the program, printing the reason and location to the console
    // and breaking into the debugger if possible.
    // This function does not return, and will exit the program with a non-zero exit code.
    template <typename... Args>
    [[noreturn]] inline void panic(_internal::panic_fmt<std::type_identity_t<Args>...> reason, Args&&... args) noexcept
        requires(sizeof...(Args) > 0)
    {
#ifdef RAOE_CORE_USE_SPDLOG
        spdlog::critical("!!!Panic!!!!\n\nReason: \"{}\"\n\nWhere:\n\t{}:{}:{}",
                         std::format(reason.m_reason, std::forward<Args>(args)...), reason.m_location.file_name(),
                         reason.m_location.line(), reason.m_location.column());
#endif
        raoe::debug::debug_break();
        std::abort();
    }
}